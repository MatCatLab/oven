Oven
====

Oven is a CI helper that should wrap test binaries calls and make sure those do
not use unexpected resources. For example, test binaries may run other (child)
processes for different testing purposes. One may want to ensure that once
initial (parent) process stops, no other testing processes are left and system
is able to run new tests which won't be affected by testing processes that were
left by previous run. Or one would like to make sure testing processes do not
consume too much CPU/RAM all together or each separately.

WARNING: THIS IS NOT IN ANY MEAN A SECURITY SANDBOX
