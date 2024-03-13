import m5
from m5.objects import *

print("sss")
system = System()
system.clk_domain = SrcClockDomain(
    clock = '1GHz',
    voltage_domain = VoltageDomain()
)
system.mem_mode = 'timing'

system.mem_ranges = [
    AddrRange("2MB"),
    AddrRange(Addr("2MB"), size="100MB"),
]

print("a")
system.nvm_ctrl = MemCtrl()
print("f")
system.nvm_ctrl.dram = DDR4_2400_16x4()
print("g")
system.nvm_ctrl.dram.range = system.mem_ranges[0]
print("h")
print(system.nvm_ctrl.dram.range)
print("i")

system.dram_ctrl = MemCtrl()
print("b")
system.dram_ctrl.dram = NVM_2400_1x64()
print("c")
system.dram_ctrl.dram.range = system.mem_ranges[1]
print("d")
print(system.dram_ctrl.dram.range)
print("e")

system.cpu = TimingSimpleCPU()
system.membus = SystemXBar()

system.cpu.icache_port = system.membus.cpu_side_ports
system.cpu.dcache_port = system.membus.cpu_side_ports
system.dram_ctrl.port = system.membus.mem_side_ports
system.nvm_ctrl.port = system.membus.mem_side_ports


system.cpu.createInterruptController()
# x86 only
system.cpu.interrupts[0].pio = system.membus.mem_side_ports
system.cpu.interrupts[0].int_requestor = system.membus.cpu_side_ports
system.cpu.interrupts[0].int_responder = system.membus.mem_side_ports

system.system_port = system.membus.cpu_side_ports

binary = "tests/test-progs/hello/bin/x86/linux/hello"
system.workload = SEWorkload.init_compatible(binary)
process = Process()
process.cmd = ["tests/test-progs/hello/bin/x86/linux/hello"]
system.cpu.workload = process
system.cpu.createThreads()

root = Root(full_system=False, system=system)
m5.instantiate()
print("Beginning simulation!")
exit_event = m5.simulate()
print("Exiting @ tick {} because {}"
    .format(m5.curTick(), exit_event.getCause()))
