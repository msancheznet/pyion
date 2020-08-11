""" 
# ===========================================================================
# Interface between the ION's implementation of the Bundle Protocol and Python. 
# Internally, all classes call the C Extension _bp.
#
# Author: Marc Sanchez Net
# Date:   04/17/2019
# Copyright (c) 2019, California Institute of Technology ("Caltech").  
# U.S. Government sponsorship acknowledged.
# ===========================================================================
"""

# General imports
from unittest.mock import Mock
import os
from pathlib import Path
from threading import Thread
from warnings import warn

# Module imports
import pyion
import pyion.utils as utils
from pyion.constants import BpEcsEnumeration

# Import C Extension
import _bp

# Define all methods/vars exposed at pyion
__all__ = ['Endpoint']

# ============================================================================
# === Endpoint object
# ============================================================================

class Endpoint():
	""" Class to represent and Endpoint. Do not instantiate it manually, use
		the Proxy's ``open``, ``close``, ``interrupt`` methods instead.

		:ivar proxy: Proxy that created this endpoint.
		:ivar eid:   Endpoint ID.
		:ivar node_dir: Directory for this node. Inherited from the proxy that
						created this endpoint object.
		:ivar _sap_addr: Memory address of the BpSapState object used by the 
						 C Extension. Do not modify or potential memory leak 						
	"""
	def __init__(self, proxy, eid, sap_addr, TTL, priority, report_eid,
				 custody, report_flags, ack_req, retx_timer, detained, 
				 chunk_size, timeout, mem_ctrl):
		""" Endpoint initializer  """
		# Store variables
		self.proxy        = proxy
		self.eid 	      = eid
		self._sap_addr    = sap_addr
		self.node_dir     = proxy.node_dir
		self.TTL          = TTL
		self.priority     = priority
		self.report_eid   = report_eid
		self.custody      = custody
		self.report_flags = report_flags
		self.ack_req 	  = ack_req
		self.retx_timer   = retx_timer
		self.chunk_size   = chunk_size
		self.detained     = detained
		self.timeout      = timeout
		self.mem_ctrl     = mem_ctrl

		# TODO: This properties are hard-coded because they are not 
		# yet supported
		self.sub_priority = 1	# Also known as ``ordinal``
		self.criticality  = ~int(BpEcsEnumeration.BP_MINIMUM_LATENCY)

		# Mark if the endpoint is blocked 
		self.tx_result = None
		self.rx_result = None

	def __del__(self):
		# Attempt to close prior to deleting
		self.close()

	@property
	def is_open(self):
		""" Returns True if this endpoint is opened """
		return (self.proxy is not None and self._sap_addr is not None)

	def _cleanup(self):
		""" Deletes eid and sap_addr information to indicate that this endpoint is closed

			.. Danger:: DO NOT call this function directly. Use the Proxy ``close``
						function instead, or the enpoint close
		"""
		self.eid  	   = None
		self._sap_addr = None
		self.proxy     = None

	def close(self):
		""" Close this endpoint if necessary. """
		if not self.is_open:
			return
		self.proxy.bp_close(self)

	@utils._chk_is_open
	@utils.in_ion_folder
	def bp_send(self, dest_eid, data, TTL=None, priority=None,
				report_eid=None, custody=None, report_flags=None,
				ack_req=None, retx_timer=None, chunk_size=None):
		""" Send data through the proxy

			:param dest_eid: Destination EID for this data
			:param data: Data to send as ``bytes``, ``bytearray`` or a ``memoryview``
			:param **kwargs: See ``Proxy.bp_open``
		"""
		# Get default values if necessary
		if TTL is None: TTL = self.TTL
		if priority is None: priority = self.priority
		if report_eid is None: report_eid = self.report_eid
		if custody is None: custody = self.custody
		if report_flags is None: report_flags = self.report_flags
		if ack_req is None: ack_req = self.ack_req
		if retx_timer is None: retx_timer = self.retx_timer
		if chunk_size is None: chunk_size = self.chunk_size

		# If this endpoint is not detained, you cannot use a retx_timer
		if retx_timer>0 and not self.detained:
			raise ConnectionError('This endpoint is not detained. You cannot set up custodial timers.')

		# Create call arguments
		args = (dest_eid, data, TTL, priority, report_eid, custody, report_flags,
				ack_req, retx_timer, chunk_size)

		# Open another thread because otherwise you cannot handle a SIGINT if blocked
		# waiting for ZCO space
		th = Thread(target=self._bp_send, args=args, daemon=True)
		th.start()

		# Wait for a bundle to be sent
		th.join()

		# If sending resulted in an exception, raise it
		if isinstance(self.tx_result, BaseException):
			raise self.tx_result

	def _bp_send(self, dest_eid, data, TTL, priority, report_eid, custody, report_flags,
				ack_req, retx_timer, chunk_size):
		""" Send data through the proxy

			:param dest_eid: Destination EID for this data
			:param data: Data to send as ``bytes``, ``bytearray`` or a ``memoryview``
			:param **kwargs: See ``Proxy.bp_open``
		"""
		# If you need to send in full, do it
		if chunk_size is None:
			self._bp_send_bundle(dest_eid, data, TTL, priority, report_eid, custody,
				 				 report_flags, ack_req, retx_timer)
			return

		# If data is a string, then encode it to get a bytes object
		if isinstance(data, str): data = data.encode('utf-8')

		# Create a memoryview object
		memv = memoryview(data)

		# Send data in chuncks of chunk_size bytes
		# NOTE: If data is not a multiple of chunk_size, the memoryview
		#  		object returns the correct end of the buffer.
		for i in range(0, len(memv), chunk_size):
			self._bp_send_bundle(dest_eid, memv[i:(i+chunk_size)].tobytes(), TTL, 
								 priority, report_eid, custody, report_flags, 
								 ack_req, retx_timer)

	def _bp_send_bundle(self, dest_eid, data, TTL, priority, report_eid, custody,
				 		report_flags, ack_req, retx_timer):
		""" Transmit a bundle """
		try:
			_bp.bp_send(self._sap_addr, dest_eid, report_eid, TTL, priority,
						custody, report_flags, int(ack_req), retx_timer, data)
			self.tx_result = None
		except BaseException as e:
			self.tx_result = e

	def bp_send_file(self, dest_eid, file_path, **kwargs):
		""" Convenience function to send a file

			.. Tip:: To send file in chunk, use ``chunk_size`` argument

			:param str: Destination EID
			:param Path or str: Path to file being sent
			:param **kwargs: See ``Proxy.bp_open``
		"""
		# Initialize variables
		file_path = Path(file_path)

		# If file does not exist, raise error
		if not file_path.exists():
			raise IOError('{} is not valid'.format(file_path))

		# Send it
		self.bp_send(dest_eid, file_path.read_bytes(), **kwargs)

	def _bp_receive(self, chunk_size):
		""" Receive one or multiple bundles.

			:param chunk_size: Number of bytes to receive at once.
		"""
		# If no chunk size defined, simply get data in the next bundle
		if chunk_size is None:
			self.rx_result = self._bp_receive_bundle()
			return

		# Pre-allocate buffer of the correct size and create a memory view
		buf  = bytearray(chunk_size)
		memv = memoryview(buf)

		# Initialize variables
		extra_bytes = None
		bytes_read  = 0

		# Read bundles as long as you have not reached the chunk size
		while self.is_open and bytes_read < chunk_size:
			# Get data from next bundle
			data = _bp.bp_receive(self._sap_addr)

			# If data is of type exception, return
			if isinstance(data, BaseException):
				self.rx_result = data
				return

			# Create memoryview to this bundle's payload
			bmem = memoryview(data)
			sz   = len(bmem)

			# Put as much data as possible in the pre-allocated buffer
			idx     = min(bytes_read+sz, chunk_size)
			to_read = idx-bytes_read
			memv[bytes_read:idx] = bmem[0:to_read]

			# If this bundle has data that does not fit in pre-allocated buffer
			# append it at then at the expense of a copy
			if to_read < sz:
				extra_bytes = bmem[to_read:]
			
			# Update the number of bytes read
			bytes_read += sz

		# If there are extra bytes add them
		self.rx_result = memv.tobytes()
		if extra_bytes: self.rx_result += extra_bytes.tobytes()

	@utils.in_ion_folder
	def _bp_receive_bundle(self):
		""" Receive one bundle """
		# Get payload from next bundle. If exception, return it too
		try:
			return _bp.bp_receive(self._sap_addr)
		except BaseException as e:
			return e

	@utils._chk_is_open
	def bp_receive(self, chunk_size=None, timeout=None):
		""" Receive data through the proxy. This is BLOCKING call. If an error
			occurred while receiving, an exception is raised.
		
			.. Tip:: The actual data reception is done in a separate thread
					 so that the user can interrupt it with a SIGINT. 
			.. Warning:: Use the chunk_size parameter with care. It is provided because each
						 ``bp_receive`` call has some overhead (e.g. a new thread is spawned),
						 so you might not want to do this if you expect very small bundles. 
						 However, if the tx sends an amount of data that is not a multiple of
						 chunk_size, then ``bp_receive`` will be blocked forever.

			:param chunk_size: If not specified, then read one bundle at a time, with
							   each bundle having a potentially different size.
							   If specified, it tells you the least number of bytes to read
							   at once (i.e., this function will return a bytes object
							   with length >= chunk_size)
			:param timeout: If not specified, then no timeout is used. If specified,
							reception of a bundle will be cancelled after timeout seconds.
		"""
		# Get default values if necessary
		if chunk_size is None: chunk_size = self.chunk_size
		if timeout is None: timeout = self.timeout

		# For now, do not let user use both chunk_size and timeout together
		# to avoid complexity
		if chunk_size and timeout:
			raise ValueError('bp_receive cannot have chunk_size and timeout at the same time.')

		# Open another thread because otherwise you cannot handle a SIGINT
		th = Thread(target=self._bp_receive, args=(chunk_size,), daemon=True)
		th.start()

		# Wait for a bundle to be delivered	and set a timeout
		th.join(timeout)

		# Handle the case where you have timed-out
		if th.is_alive():
			self._bp_receive_timeout()
			return self.rx_result

		# If exception, raise it
		if isinstance(self.rx_result, BaseException):
			raise self.rx_result

		return self.rx_result

	def _bp_receive_timeout(self):
		# Interrupt the reception of bundles from ION. 
		self.proxy.bp_interrupt(self)

	def __enter__(self):
		""" Allows an endpoint to be used as context manager """
		return self

	def __exit__(self, exc_type, exc_val, exc_tb):
		""" Allows an endpoint to be used as context manager """
		self.close()

	def __str__(self):
		return '<Endpoint: {} ({})>'.format(self.eid, 'Open' if self.is_open else 'Closed')

	def __repr__(self):
		return '<Endpoint: {} ({})>'.format(self.eid, self._sap_addr)

