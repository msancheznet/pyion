:: =====================================================================
:: Start a network of Docker containers each of which
:: has an ION node.
::
:: Parameters
:: ----------
::   %1 = Name of the docker-compose file (default is
::        ion_compose.yaml)
::
:: Usage
:: -----
:: C:\>.\connect_to 1
:: root@ion_node1:~#
::
:: Author: Marc Sanchez Net
:: 01/07/2019
:: Copyright (c) 2019, Jet Propulsion Laboratory.
:: =====================================================================
@echo off

:: Set Docker-compose file
set filename="./bp_tests/test_network.yaml"

:: Clean up docker
docker rm ion_node1
docker rm ion_node2
docker system prune -f

:: Start all ION nodes
docker-compose -f %filename% up

:: List all nodes that are available in system
docker ps