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
from pyion.constants import BpEcsEnumeration, BpCustodyEnum

# Import C Extension
import _bp

# Define all methods/vars exposed at pyion
__all__ = ['Endpoint', 'is_bpv6', 'is_bpv7']

# Get version of BP used in this deployment of pyion
bp_version = os.environ.get('PYION_BP_VERSION')
if not bp_version:
    raise ValueError("Environment variable PYION_BP_VERSION is missing. Set it to BPv6 or BPv7")
if bp_version.lower() not in ['bpv7', 'bpv6']:
	raise ValueError("Environment variable PYION_BP_VERSION must be set to BPv6 or BPv7")
is_bpv6 = bp_version.lower() == 'bpv6'
is_bpv7 = bp_version.lower() == 'bpv7'

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
				 chunk_size, timeout, mem_ctrl, return_headers):
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
		self.return_headers = return_headers

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
		if retx_timer>0 and is_bpv6 and not self.detained:
			raise ConnectionError('This endpoint is not detained. You cannot set up custodial timers.')

		# If using BPv7, custody transfer is not defined
		if is_bpv7 and custody != BpCustodyEnum.NO_CUSTODY_REQUESTED:
			raise ValueError('Custody transfer is not allowed in pyion-4.0.0+ since it is not in BPv7')


		# Reset of transmit result result
		self.tx_result = None

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

	@utils._chk_is_open
	def bp_receive(self, timeout=None, return_headers=None):
		""" Receive data through the proxy. This is BLOCKING call. If an error
			occurred while receiving, an exception is raised.
		
			.. Tip:: The actual data reception is done in a separate thread
					 so that the user can interrupt it with a SIGINT. 

			:param timeout: If not specified, then no timeout is used. If specified,
							reception of a bundle will be cancelled after timeout seconds.
			:param return_headers: If set to True, the return value of bp_receive will be a
									tuple where the first value is the payload and the second
									is a dictionary containing the bundle header information.
									This value defaults to False. 
		"""
		# Get default values if necessary
		if timeout is None: timeout = self.timeout
		if return_headers is None: return_headers = self.return_headers

		# Reset of receive result
		self.rx_result = None

		# Open another thread because otherwise you cannot handle a SIGINT
		th = Thread(target=self._bp_receive, args=(return_headers,), daemon=True)
		th.start()

		# Wait for a bundle to be delivered	and set a timeout. Upon timeout expiration
		# if the thread is still alive, then reception via ION must be interrupted.
		th.join(timeout)
		if th.is_alive():
			self._bp_receive_timeout()
			return

		# If exception, raise it
		if isinstance(self.rx_result, BaseException):
			raise self.rx_result

		return self.rx_result

	@utils.in_ion_folder
	def _bp_receive(self, return_headers):
		""" Receive one bundle.

			:param return_headers: If true, will return bundle headers.
		"""
		# Get payload from next bundle. If exception, return it too
		try:
			self.rx_result = _bp.bp_receive(self._sap_addr, int(return_headers))
		except BaseException as e:
			self.rx_result = e
		
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

