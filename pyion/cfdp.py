""" 
# ===========================================================================
# Interface between the ION's implementation of the CCSDS CFDP and Python. 
# Internally, all classes call the C Extension _cfdp.
#
# Author: Marc Sanchez Net
# Date:   04/17/2019
# Copyright (c) 2019, California Institute of Technology ("Caltech").  
# U.S. Government sponsorship acknowledged.
# ===========================================================================
"""

# General imports
from unittest.mock import Mock
from pathlib import Path
from threading import Event, Thread
from warnings import warn

# Module imports
import pyion
import pyion.utils as utils
from pyion.constants import CfdpEventEnum

# Import C Extension
import _cfdp

# Define all methods/vars exposed at pyion
__all__ = ['Entity']

# ============================================================================
# === Entity class
# ============================================================================

class Entity():
	""" Class to represent a CFDP entity. """
	def __init__(self, proxy, peer_entity_nbr, param_addr, endpoint, mode,
				 closure_latency, seg_metadata):
		""" Class initializer """
		# Store variables
		self.proxy        = proxy
		self.entity_nbr   = peer_entity_nbr
		self.node_dir     = proxy.node_dir
		self._param_addr  = param_addr
		self.endpoint     = endpoint
		self.mode         = mode
		self.closure_lat  = closure_latency
		self.seg_metadata = seg_metadata

		# Map {event_id}: event handler function
		self.event_handers = {}

		# Create a condition to signal end of transaction
		self._end_transaction = Event()
		self._ok_transaction = False

		# Start a thread to monitor all events
		self.th = Thread(target=self._monitor_events, daemon=False)
		self.th.start()

	def __del__(self):
		# If you have already been closed, return
		if not self.is_open:
			return

		# Close the Entity and free C memory
		self.proxy.cfdp_close(self.entity_nbr)

	@property
	def is_open(self):
		""" Returns True if entity is opened """
		return (self.proxy is not None and self._param_addr is not None)

	def _cleanup(self):
		""" Clean entity after closing. Do not call directly,
			use ``proxy.cfdp_close``
		"""
		# Clear variables
		self.proxy       = None
		self.entity_nbr  = None
		self._param_addr = None
		self.endpoint    = None

		# Stop the thread monitoring CFDP events
		_cfdp.cfdp_interrupt_events()

		# Wait until the event thread has finished
		self.th.join()

		# Mark end of transactions to wake up threads
		self._mark_transaction_end(False)
		
	@utils._chk_is_open
	@utils.in_ion_folder
	def cfdp_send(self, source_file, dest_file=None, mode=None,
				  closure_lat=None, seg_metadata=None):
		""" Send a file using CFDP to the peer engine for this entity

			:param source_file: str or Path of file to send
			:param dest_file: str or Path. Name of file at receiving
							  engine. It defaults to source_file
			:param **kwargs: See ``proxy.cfdp_send``
		"""
		# Create path object
		src_file = Path(source_file)

		# Check that the source file exists
		if not src_file.exists():
			raise FileNotFoundError('Source file {} does not exist'.format(src_file))

		# Initialize variables
		src_file = src_file.resolve().absolute()

		# Set default values
		dst_file = source_file if dest_file is None else dest_file
		
		# Set default values if necessary
		if mode is None: mode = self.mode
		if closure_lat is None: closure_lat = self.closure_lat
		if seg_metadata is None: seg_metadata = self.seg_metadata

		# Mark that the current transaction has not succeeded yet
		self._ok_transaction = False

		# Trigger CFDP send
		_cfdp.cfdp_send(self._param_addr, str(src_file), str(dst_file), 
							  closure_lat, seg_metadata, mode)

	@utils._chk_is_open
	@utils.in_ion_folder
	def cfdp_request(self, source_file, dest_file=None, mode=None,
				  	 closure_lat=None, seg_metadata=None):
		""" Request a file to be sent to this node using CFDP

			:param source_file: str or Path of file to request
			:param dest_file: str or Path. Name of file at this node 
							  once it is received. Defaults to ``source_file``
			:param **kwargs: See ``proxy.cfdp_send``
		"""
		# Set default values if necessary
		if mode is None: mode = self.mode
		if closure_lat is None: closure_lat = self.closure_lat
		if seg_metadata is None: seg_metadata = self.seg_metadata

		# Mark that the current transaction has not succeeded yet
		self._ok_transaction = False

		# Trigger CFDP request
		_cfdp.cfdp_request(self._param_addr, str(source_file), str(dest_file), 
								 closure_lat, seg_metadata, mode)

	@utils._chk_is_open
	@utils.in_ion_folder
	def cfdp_cancel(self):
		""" Cancel the current CFDP transaction """
		_cfdp.cfdp_cancel(self._param_addr)

	@utils._chk_is_open
	@utils.in_ion_folder
	def cfdp_suspend(self):
		""" Suspend the current CFDP transaction """
		_cfdp.cfdp_suspend(self._param_addr)

	@utils._chk_is_open
	@utils.in_ion_folder
	def cfdp_resume(self):
		""" Resume the current CFDP transaction """
		_cfdp.cfdp_resume(self._param_addr)

	@utils._chk_is_open
	@utils.in_ion_folder
	def cfdp_report(self):
		""" Request issuance on the transmission/reception progress of the current CFDP
			transaction.
		"""
		_cfdp.cfdp_report(self._param_addr)

	@utils._chk_is_open
	def add_usr_message(self, msg):
		""" Add a user message to all CFDP PDUs in the next transaction 

			:param str: User message to add
		"""
		_cfdp.cfdp_add_usr_msg(self._param_addr, msg)

	@utils._chk_is_open
	def add_filestore_request(self, action, file1, file2=None):
		""" Add a user message to all CFDP PDUs in the next transaction 

			:param action: See ``pyion.CFDP_CREATE_FILE``, etc.
			:param file1: String or Path-object
			:param file2: None, string or Path-object.
		"""
		# Transform files to string
		file1 = str(file1)
		if file2 is not None: file2 = str(file2)

		# Call pyion function
		_cfdp.cfdp_add_filestore_request(self._param_addr, action, file1, file2)
	
	def register_event_handler(self, event, func):
		""" Register and event handler for this entity

			:param event: See ``pyion.constants.CFDP_CREATE_FILE``, etc.
			:param func: Function handle with the following signature
						 ``def ev_handler(params)``, where params is a 
						 dictionary with contents that depend on the type
						 of event (see CCSDS CFDP spec, section 3.5.6)
		"""
		self.event_handers[event] = func

	def wait_for_transaction_end(self, timeout=None):
		""" Blocks the calling thread until the transaction has
			finished or the timeout expires.

			:param timeout: Time to wait in [seconds]
			:return: True if transaction finished successfully
		"""
		# Wait for the end of the transaction
		self._end_transaction.wait(timeout=timeout)

		# Return state of transaction
		return self._ok_transaction

	def _mark_transaction_end(self, success):
		""" Mark that the current CFDP transaction has ended 
		
			:param success: True/False
		"""
		# Mark the transaction as successful
		self._ok_transaction = success

		# Awake all threads that were waiting
		self._end_transaction.set()

		# Reset the event
		self._end_transaction.clear()

	def _monitor_events(self):
		""" Monitor all CFDP events """
		while self.is_open:
			# Get the next event
			evt, ev_params = _cfdp.cfdp_next_event()

			# Create event type class from integer code
			evt = CfdpEventEnum(evt)

			# If no event, continue
			if evt == CfdpEventEnum.CFDP_NO_EVENT:
				continue

			# If you have an event handler for this event type, use it
			try:
				func = self.event_handers[evt]
				func(evt, ev_params)
			except KeyError:
				pass

			# If you have a handler for all events, use it
			try:
				func = self.event_handers[CfdpEventEnum.CFDP_ALL_IND]
				func(evt, ev_params)
			except KeyError:
				pass

			# If transaction finished ok, report it
			if evt == CfdpEventEnum.CFDP_TRANSACTION_FINISHED_IND:
				self._mark_transaction_end(True)

			# If transaction failed, report it
			if evt == CfdpEventEnum.CFDP_ABANDONED_IND:
				self._mark_transaction_end(False)
	
	def __enter__(self):
		""" Allows an endpoint to be used as context manager """
		return self

	def __exit__(self, exc_type, exc_val, exc_tb):
		""" Allows an endpoint to be used as context manager """
		pass

	def __str__(self):
		return '<Entity: {} ({})>'.format(self.entity_nbr, 'Open' if self.is_open else 'Closed')

	def __repr__(self):
		return '<Entity: {} ({})>'.format(self.entity_nbr, self._param_addr)