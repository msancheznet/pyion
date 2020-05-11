#!/usr/bin/python3

"""
# Python wrapper for ``netem``. Its main goal is to simplify managing
# network emulation rules by exposing a simple (albeit limited) Python
# interface
#
# See ``http://man7.org/linux/man-pages/man8/tc-netem.8.html``
#
# Usage Example
# -------------
# >>> import pynetem as netem
# >>> 
# >>> netem.list_interfaces()
# ['lo', 'tunl0', 'ip6tnl0', 'eth0']
# >>> p = netem.Proxy('eth0')
# >>> p.add_delay_rule()
# >>> p.add_loss
# >>> p.add_random_loss_rule()
# >>> p.command
# 'tc qdisc add dev eth0 root netem delay 100ms loss 1.0%'
# >>> p.commit_rules()
# >>> p.show_rules()
# qdisc netem 800e: root refcnt 2 limit 1000 delay 100.0ms loss 1%
#
# Author: Marc Sanchez Net
# Date:   05/17/2019
# Copyright 2019 (c), Jet Propulsion Laboratory
"""

from functools import wraps
import os
import shlex
import subprocess

# ============================================================================
# === Default values
# ============================================================================

# Default values for Delay Rule
DEFAULT_DEV    = 'eth0'
DEFAULT_DELAY  = 100.0
DEFAULT_UNITS  = 'ms'
DEFAULT_JITTER = 0.0
DEFAULT_CORR   = 0.0
DEFAULT_DIST   = None   # Defaults to normal 

# Default Error Probability
DEFAULT_P_ERR = 0.01

# Default values for Gilbert-Elliot Model
DEFAULT_GE_P   = 0.05
DEFAULT_GE_R   = 0.95
DEFAULT_GE_1_K = 0.0
DEFAULT_GE_1_H = 1.0

# Default distance between two reordered packets
DEFAULT_DISTANCE = -1

# ============================================================================
# === Helper functions
# ============================================================================

def _exec(raise_exc=True):
    def _exec(func):
        @wraps(func)
        def wrapper(*args, **kwargs):
            # Get the command
            command = func(*args, **kwargs)

            # Execute the command
            if raise_exc:
                subprocess.check_call(shlex.split(command))
            else:
                subprocess.call(shlex.split(command))
        return wrapper
    return _exec
    
def _build_cmd(func):
    @wraps(func)
    def wrapper(self, *args, **kwargs):
        # Get the next part of the command
        cmd = func(self, *args, **kwargs)

        # If command is not initialized, do it
        if not self._is_initialized:
            self._initialize()

        # Append the next part of the command
        self.command += (' ' + cmd)
    return wrapper

# ============================================================================
# === Main functions
# ============================================================================

def list_interfaces():
    """ Returns list of network interfaces in the system """
    return os.listdir('/sys/class/net/')

class Proxy():
    """ Proxy to netem. It allows you to build a rule and then commit it. """     
    def __init__(self, device=DEFAULT_DEV):
        # Store variables
        self.device  = device
        self.command = ''

    @property
    def _is_initialized(self):
        return self.command.startswith('tc qdisc add dev')

    def _initialize(self):
        self.command = 'tc qdisc add dev {} root netem'.format(self.device)

    def reset(self):
        # Reset command
        self.command = ''

        # Delete all current rules
        self.delete_rules()

    @_exec
    def commit_rules(self):
        # Delete all current rules
        self.delete_rules()

        # Trigger execution of new rules
        return self.command

    @_exec(raise_exc=False)
    def delete_rules(self):
        return 'tc qdisc del dev {} root'.format(self.device)

    @_exec
    def show_rules(self):
        return 'tc qdisc show dev {}'.format(self.device)

    @_build_cmd
    def add_delay_rule(
        self,
        delay=DEFAULT_DELAY,
        jitter=DEFAULT_JITTER,
        units=DEFAULT_UNITS,
        correlation=DEFAULT_CORR,
        distribution=DEFAULT_DIST
    ):
        """ Add a rule to delay traffic leaving this node (i.e., its egress plan)
            
            :param float delay: Delay as a number
            :param float jitter: Variation of delay as a number
            :param str units: Delay/Jitter units
            :param float correlation: Delay autocorrelation as number [0, 1]
            :param str dist: Delay distribution from {'uniform', 'normal', 'pareto', 'paretonormal'}
        """
        # Initialize command
        cmd = 'delay {}{}'.format(delay, units)

        # If correlation or distribution are specfied but jitter is not, add a minimal jitter.
        # This is due to the fact that otherwise netem fails to parse the arguments correctly
        if correlation > 0 or distribution is not None:
            jitter, j_units = 0.001, 'ms'
        else:
            j_units = units

        # If jitter is >= 0.001, add it. Less than that value does not work. Also, if jitter
        # is not provided, then the parameters afterwards are not properly parsed
        if jitter >= 0.001:
            cmd += ' {}{}'.format(jitter, j_units)

        # Add delay correlation if specified
        if correlation > 0:
            cmd += ' {}%'.format(100*correlation)

        # Add distribution
        if distribution is not None:
            cmd += ' distribution {}'.format(distribution)

        # Return delay part of the command
        return cmd

    @_build_cmd
    def add_random_loss_rule(self, error_prob=DEFAULT_P_ERR):
        """ Add a rule to add random loss to the traffic leaving this node

            .. Warning:: In the netem documentation you can also add a correlation
                        to the loss. However, this is deprecated due to 

            :param float percent: Loss percentage [0, 1]
        """
        return 'loss {}%'.format(100*error_prob)

    @_build_cmd    
    def add_bernoulli_loss_rule(self, error_prob=DEFAULT_P_ERR):
        """ Model packet losses as a Bernoulli random variable

            :param float error_prob: Packet error probability [0, 1]
        """
        return 'loss gemodel {}%'.format(100*error_prob)

    @_build_cmd
    def add_simple_gilbert_elliot_loss_rule(
        self,
        p=DEFAULT_GE_P,
        r=DEFAULT_GE_R
    ):
        """ Simplified Gilbert-Elliot Model. In Good state, the packet error probability is 0; 
            in Bad state it is 1.

            .. Tip:: See ``http://www.ohohlfeld.com/paper/hasslinger_hohlfeld-mmb_2008.pdf``

            :param float p: Probability of transitioning from Good to Bad state [0, 1]
            :param float r: Probability of transitioning from Bad to Good state [0, 1]
        """
        return 'loss gemodel {}% {}%'.format(100*p, 100*r)

    @_build_cmd
    def add_gilbert_elliot_loss_rule(
        self,
        p=DEFAULT_GE_P,
        r=DEFAULT_GE_R,
        u_k=DEFAULT_GE_1_K,
        u_h=DEFAULT_GE_1_H
    ):
        """ Gilbert-Elliot Model. In Good state, the packet error probability is 1-h; 
            in Bad state it is 1-k.

            .. Tip:: See ``http://www.ohohlfeld.com/paper/hasslinger_hohlfeld-mmb_2008.pdf``

            :param float p: Probability of transitioning from Good to Bad state [0, 1]
            :param float r: Probability of transitioning from Bad to Good state [0, 1]
            :param float u_k: Probability of error in Good state [0, 1]
            :param float u_h: Probability of error in Bad state [0, 1]
        """
        return 'loss gemodel {}% {}% {}% {}%'.format(100*p, 100*r, 100*u_k, 100*u_h)

    @_build_cmd
    def add_corrupt_rule(
        self,
        err_prob=DEFAULT_P_ERR,
        correlation=DEFAULT_CORR
    ):
        """ Introduce errors in random positions within a packet.

            :param err_prob: Error probability [0, 1]
            :param correlation: Correlation between two consecutive errors [0, 1]
        """
        return 'corrupt {}% {}%'.format(100*err_prob, 100*correlation)

    @_build_cmd
    def add_duplicate_rule(
        self,
        dup_prob=DEFAULT_P_ERR,
        correlation=DEFAULT_CORR
    ):
        """ Duplicate packets before enqueuing them for departure in this node.

            :param dup_prob: Duplication probability [0, 1]
            :param correlation: Correlation between two consecutive duplications [0, 1]
        """
        return 'duplicate {}% {}%'.format(100*dup_prob, 100*correlation)

    @_build_cmd
    def add_reorder_rule(
        self,
        reorder_prob=DEFAULT_P_ERR,
        correlation=DEFAULT_CORR,
        distance=DEFAULT_DISTANCE
    ):
        """ Reorder packets. If no distance is provided, then basic reordering 
            with a given probability and correlation. For meaning of distance 
            parameter, see ``http://man7.org/linux/man-pages/man8/tc-netem.8.html``

            :param float reorder_prob: Reorder probability
            :param float correlation: Correlation between consecutive reordering events
            :param float distance:
        """
        # Initialize command
        cmd = 'reorder {}% {}%'.format(reorder_prob, correlation)

        # If no distance, return
        if distance == -1:
            return cmd

        # Add the gap parameter
        return (cmd + ' gap {}'.format(distance))

    def __str__(self):
        return '<NetemProxy dev={}>'.format(self.device)

    def __repr__(self):
        return str(self)