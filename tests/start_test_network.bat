:: =====================================================================
:: Start a network of Docker containers. Each of which has an ION node.
::
:: Parameters
:: ----------
::   %1 = Name of test folder. Defaults to ``bp_tests``
::
:: Usage
:: -----
:: C:\> start_test_network bp_tests
:: C:\> start_test_network ltp_tests
:: C:\> start_test_network cfdp_tests
::
:: Author: Marc Sanchez Net
:: 01/07/2019
:: Copyright (c) 2019, Jet Propulsion Laboratory.
:: =====================================================================
@echo off

:: Set Docker-compose file
if "%1"=="" (
	set filename="./bp_tests/test_network.yaml"
) else (
	set filename="./%1/test_network.yaml"
)
echo %filename%

:: Clean up docker
docker rm ion_node1
docker rm ion_node2
docker system prune -f

:: Start all ION nodes
docker-compose -f %filename% up

:: List all nodes that are available in system
docker ps