/* Simulates a Windows executable (PE) using WINE on Linux */
#include "rose.h"
#include "RSIM_Private.h"

#ifdef ROSE_ENABLE_SIMULATOR /* protects this whole file */

#include "RSIM_Linux32.h"
#include "RSIM_Adapter.h"

/* Simulate RDTSC instruction by prividing values obtained from debugging. */
class Rdtsc: public RSIM_Callbacks::InsnCallback {
public:
    virtual Rdtsc *clone() { return this; }
    /* The TSC is based on the number of instructions simulated and a manual adjustment. */
    static const uint64_t start = 0xf95654a57da20ull;   // arbitrary
    size_t index;
    std::vector<int> adjustment;
    Rdtsc(): index(0) {
        adjustment.push_back(0);
        adjustment.push_back(0);        adjustment.push_back(0);
        adjustment.push_back(0);        adjustment.push_back(-5);
        adjustment.push_back(0);        adjustment.push_back(-10);
        adjustment.push_back(0);        adjustment.push_back(-15);
        adjustment.push_back(0);        adjustment.push_back(0);
    }
    uint64_t next_value(const Args& args) {
        return start + 1234 * args.thread->policy.get_ninsns() + (index < adjustment.size() ? adjustment[index++] : 0);
    }

    virtual bool operator()(bool enabled, const Args &args) {
        SgAsmx86Instruction *insn = isSgAsmx86Instruction(args.insn);
        if (insn && x86_rdtsc==insn->get_kind()) {
            uint32_t newip = insn->get_address() + insn->get_size();
            uint64_t value = next_value(args);
            uint32_t eax = value;
            uint32_t edx = value>>32;
            args.thread->policy.writeRegister(args.thread->policy.reg_eax, args.thread->policy.number<32>(eax));
            args.thread->policy.writeRegister(args.thread->policy.reg_edx, args.thread->policy.number<32>(edx));
            args.thread->policy.writeRegister(args.thread->policy.reg_eip, args.thread->policy.number<32>(newip));
            enabled = false;
            args.thread->tracing(TRACE_MISC)->mesg("RDTSC adjustment; returning 0x%016"PRIx64, value);
        }
        return enabled;
    }
};

/* wine-pthread tries to open /proc/self/maps.  This won't work because that file describes the simulator rather than the
 * specimen.  Therefore, when this occurs, we create a temporary file and initialize it with information from the specimen's
 * MemoryMap and open that instead.  NOTE: This appears to only happen during error reporting. */
class ProcMapsOpen: public RSIM_Simulator::SystemCall::Callback {
public:
    bool operator()(bool enabled, const Args &args) {
        RSIM_Thread *t = args.thread;
        if (enabled) {
            uint32_t filename_va = t->syscall_arg(0);
            bool error;
            std::string filename = t->get_process()->read_string(filename_va, 0, &error);
            if (error) {
                t->syscall_return(-EFAULT);
                return enabled;
            }

            if (filename=="/proc/self/maps") {
                RTS_WRITE(t->get_process()->rwlock()) {
                    FILE *f = fopen("x-maps", "w");
                    assert(f);

                    const MemoryMap::Segments &segments = t->get_process()->get_memory().segments();
                    for (MemoryMap::Segments::const_iterator si=segments.begin(); si!=segments.end(); ++si) {
                        const Extent &range = si->first;
                        const MemoryMap::Segment &segment = si->second;
                        unsigned p = segment.get_mapperms();
                        fprintf(f, "%08"PRIx64"-%08"PRIx64" %c%c%cp 00000000 00:00 0 %s\n",
                                range.first(), range.last()+1,
                                (p & MemoryMap::MM_PROT_READ)  ? 'r' : '-',
                                (p & MemoryMap::MM_PROT_WRITE) ? 'w' : '-',
                                (p & MemoryMap::MM_PROT_EXEC)  ? 'x' : '-',
                                segment.get_name().c_str());
                    }
                    fclose(f);
                    filename = "x-maps";
                } RTS_WRITE_END;

                uint32_t flags=t->syscall_arg(1), mode=(flags & O_CREAT)?t->syscall_arg(2):0;
                int fd = open(filename.c_str(), flags, mode);
                if (-1==fd) {
                    t->syscall_return(-errno);
                    return enabled;
                }
                t->syscall_return(fd);
                return false; // this implementation was successful; skip the rest
            }
        }
        return enabled;
    }
};

class MapReporter: public RSIM_Simulator::SystemCall::Callback {
public:
    virtual bool operator()(bool enabled, const Args &args) {
        if (enabled) {
            RSIM_Process *p = args.thread->get_process();
            p->mem_showmap(args.thread->tracing(TRACE_MISC), "MapReporter triggered: current memory map:", "    ");
        }
        return enabled;
    }
};

class FutexFooler: public RSIM_Simulator::SystemCall::Callback {
public:
    virtual bool operator()(bool enabled, const Args &args) {
        if (enabled && 0x68000832==args.thread->policy.readRegister<32>(args.thread->policy.reg_eip).known_value()) {
            args.thread->tracing(TRACE_MISC)->mesg("FutexFooler triggered: returning zero");
            args.thread->syscall_return(0);
            enabled = false;
        }
        return enabled;
    }
};

int
main(int argc, char *argv[], char *envp[])
{
    /**************************************************************************************************************************
     * Create simulator, including setting up all necessary callbacks.  The specimen is not actually loaded into memory until
     * later.
     **************************************************************************************************************************/
    RSIM_Linux32 sim;
    int n = sim.configure(argc, argv, envp);

#if 1
    /* Disassemble when we hit the dynamic linker at 0x68000850 */
    RSIM_Tools::MemoryDisassembler disassembler(0x68000850, false);
    sim.install_callback(&disassembler);
    sim.install_callback(new RSIM_Tools::FunctionReporter);
#elif 0
    RSIM_Tools::MemoryDisassembler disassembler(0x080a4f28, false);
    sim.install_callback(&disassembler);
    sim.install_callback(new RSIM_Tools::FunctionReporter(true));
#endif

    /* Emulate instructions that ROSE doesn't handle yet. */
    sim.install_callback(new RSIM_Tools::UnhandledInstruction);

    /* Make adjustments for RDTSC instruction after the instruction executes. */
    sim.get_callbacks().add_insn_callback(RSIM_Callbacks::AFTER, new Rdtsc);

    /* System call handlers */
    sim.syscall_implementation(5/*open*/)->body.prepend(new ProcMapsOpen);
    sim.syscall_implementation(243/*set_thread_area*/)->body.prepend(new MapReporter);
    //sim.syscall_implementation(0xf0/*futex*/)->body.prepend(new FutexFooler);

    /**************************************************************************************************************************
     * Load the specimen into memory, create a process and its initial thread.
     **************************************************************************************************************************/
    
    sim.exec(argc-n, argv+n);
    RSIM_Process *proc = sim.get_process();
    assert(proc);
    RSIM_Thread *thread = proc->get_thread(getpid()); /* main thread */
    assert(thread);

    /* Initialize the stack from data saved when the program was run natively. */
    {
#if 1
        rose_addr_t bottom_of_stack = 0xbffeb000;
        rose_addr_t new_stack_pointer = bottom_of_stack + 0x13f40; /* points to specimen's argc */
        const char *stack_name = "x-real-initial-stack";
#elif 0
        rose_addr_t bottom_of_stack   = 0xbfffe000;
        rose_addr_t new_stack_pointer = 0xbfffefd0;
        const char *stack_name = "x-a.out-stack";
#else
        rose_addr_t bottom_of_stack = 0;
        rose_addr_t new_stack_pointer = 0;
        const char *stack_name = "";
#endif

        int fd = open(stack_name, O_RDONLY);
        if (fd>=0) {
            fprintf(stderr, "Initializing the stack...\n");
            uint8_t buf[4096];
            ssize_t nread;
            while ((nread=read(fd, buf, sizeof buf))>0) {
                size_t nwritten = proc->mem_write(buf, bottom_of_stack, (size_t)nread);
                assert(nwritten==(size_t)nread);
                bottom_of_stack += nwritten;
            }
            close(fd);
            thread->policy.writeRegister(thread->policy.reg_esp, thread->policy.number<32>(new_stack_pointer));
        }
    }

    /**************************************************************************************************************************
     * Allow the specimen to run
     **************************************************************************************************************************/

    sim.activate();
    sim.main_loop();
    sim.deactivate();
    sim.describe_termination(stderr);
    sim.terminate_self(); // probably doesn't return
    return 0;
}





#else
int main(int, char *argv[])
{
    std::cerr <<argv[0] <<": not supported on this platform" <<std::endl;
    return 0;
}

#endif /* ROSE_ENABLE_SIMULATOR */
