import m5
from m5.objects import *
from m5.stats import periodicStatDump
from m5.util import addToPath

addToPath("../")
import argparse
import math

from common import (
    MemConfig,
    ObjectList,
)

# Make the system
system = System()

# Set its clock frequency and voltage
system.clk_domain = SrcClockDomain()
system.clk_domain.clock = "2GHz"
system.clk_domain.voltage_domain = VoltageDomain()

# Use timing mode for mmeory simulation
system.mem_mode = "timing"
# system.mem_ranges = [AddrRange('512MB')]
system.mem_ranges = [
    AddrRange("3MB"),
    AddrRange(Addr("3MB"), size="100MB"),
]

# Create CPU model. This one executes each instruction in a single clock cycle. (Except memory instr.)
system.cpu = TimingSimpleCPU()

# Create system-wide memory bus
system.membus = SystemXBar()

# Connect cache ports on CPU to system bus. (Our system doesn't have caches)
system.cpu.icache_port = system.membus.cpu_side_ports
system.cpu.dcache_port = system.membus.cpu_side_ports

# Connect interrupt ports to membus. This is need in x86 ISA, not others
system.cpu.createInterruptController()
system.cpu.interrupts[0].pio = system.membus.mem_side_ports
system.cpu.interrupts[0].int_requestor = system.membus.cpu_side_ports
system.cpu.interrupts[0].int_responder = system.membus.mem_side_ports

system.system_port = system.membus.cpu_side_ports


# Connect memory controller (DDR3 here) to membus
# system.HeteroMemCtrl = HeteroMemCtrl()
# system.HeteroMemCtrl.dram = DDR4_2400_16x4()
# system.HeteroMemCtrl.dram.range = system.mem_ranges[0]
# system.HeteroMemCtrl.nvm = NVM_2400_1x64()
# system.HeteroMemCtrl.nvm.range = system.mem_ranges[1]
# system.HeteroMemCtrl.port = system.membus.mem_side_ports

system.mem_ctrls = MemCtrl()
system.mem_ctrls.dram = DDR4_2400_16x4()
system.mem_ctrls.dram.range = system.mem_ranges[0]
system.mem_ctrls.port = system.membus.mem_side_ports





binary = "tests/test-progs/hello/bin/x86/linux/test"
system.workload = SEWorkload.init_compatible(binary)

# Create process to run the hello world program
process = Process()
process.cmd = ["tests/test-progs/hello/bin/x86/linux/test"]
system.cpu.workload = process
system.cpu.createThreads()

# Instantiate oython class
root = Root(full_system=False, system=system)
root.system.mem_mode = "timing"
m5.instantiate()
m5.simulate()
print("finish")

#########################################################################################
# Run simulation
print("Simulation starting in 3..2..1..")
# exit_event = m5.simulate()

# Simulation finished
# print ('Exiting @ tick %i because %s' % (m5.curTick(), exit_event.getCause()))
