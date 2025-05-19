# fedora-xdp-tools

1. Running the container on the AMD host:

```bash
# oc create -f xdp-tools-test.yaml 
namespace/xdp-tools-test created
securitycontextconstraints.security.openshift.io/xdp-scc created
networkattachmentdefinition.k8s.cni.cncf.io/xdp-net created
configmap/xdp-redirect-code created
serviceaccount/xdp-user created
pod/xdp-a created
pod/xdp-b created
```

2. Checking the `xdp-a` and `xdp-b` pods are running properly:
```bash
# oc get pods -n xdp-tools-test
NAME    READY   STATUS    RESTARTS   AGE
xdp-a   1/1     Running   0          7s
xdp-b   1/1     Running   0          7s

# oc get pods -n xdp-tools-test -o wide
NAME    READY   STATUS    RESTARTS   AGE    IP            NODE                                     NOMINATED NODE   READINESS GATES
xdp-a   1/1     Running   0          6m6s   10.132.2.24   cnfdc13.t5g-dev.eng.rdu2.dc.redhat.com   <none>           <none>
xdp-b   1/1     Running   0          6m6s   10.132.2.25   cnfdc13.t5g-dev.eng.rdu2.dc.redhat.com   <none>           <none>

```

3. Checking the `xdpdump cli`:

```bash
# oc exec -it xdp-a -n xdp-tools-test -- xdpdump -D
Defaulted container "main" out of: main, xdp-setup (init)
Interface        Prio  Program name      Mode     ID   Tag               Chain actions
--------------------------------------------------------------------------------------
lo                     <No XDP program loaded!>
eth0                   <No XDP program loaded!>
net1                   xdp_redirect_prog skb      445  e46c946fc75ace10 

command terminated with exit code 1
# oc exec -it xdp-b -n xdp-tools-test -- xdpdump -D
Defaulted container "main" out of: main, xdp-setup (init)
Interface        Prio  Program name      Mode     ID   Tag               Chain actions
--------------------------------------------------------------------------------------
lo                     <No XDP program loaded!>
eth0                   <No XDP program loaded!>
net1                   xdp_redirect_prog skb      440  e46c946fc75ace10 

command terminated with exit code 1

# oc exec -it xdp-b -n xdp-tools-test -- xdpdump -i net1 --rx-capture exit,entry
Defaulted container "main" out of: main, xdp-setup (init)
listening on net1, ingress XDP program ID 440 func xdp_redirect_prog, capture mode entry/exit, capture size 262144 bytes
^C
0 packets captured
0 packets dropped by perf ring
```

4. Checking the `xdp-tools` rpm version :

```bash
# oc exec -it xdp-b -n xdp-tools-test -- rpm -qa | grep xdp-tools
Defaulted container "main" out of: main, xdp-setup (init)
xdp-tools-1.5.4-1.fc42.x86_64
```

5. Checkinig the `cnfdc13.t5g-dev.eng.rdu2.dc.redhat.com` CPU specs:

```bash
# oc debug nodes/cnfdc13.t5g-dev.eng.rdu2.dc.redhat.com -- chroot /host lscpu
Temporary namespace openshift-debug-gh7zb is created for debugging node...
Starting pod/cnfdc13t5g-devengrdu2dcredhatcom-debug-h5hj5 ...
To use host binaries, run `chroot /host`
Architecture:                         x86_64
CPU op-mode(s):                       32-bit, 64-bit
Address sizes:                        52 bits physical, 57 bits virtual
Byte Order:                           Little Endian
CPU(s):                               384
On-line CPU(s) list:                  0-383
Vendor ID:                            AuthenticAMD
BIOS Vendor ID:                       AMD
Model name:                           AMD EPYC 9654 96-Core Processor
BIOS Model name:                      AMD EPYC 9654 96-Core Processor                
CPU family:                           25
Model:                                17
Thread(s) per core:                   2
Core(s) per socket:                   96
Socket(s):                            2
Stepping:                             1
BogoMIPS:                             4800.22
Flags:                                fpu vme de pse tsc msr pae mce cx8 apic sep mtrr pge mca cmov pat pse36 clflush mmx fxsr sse sse2 ht syscall nx mmxext fxsr_opt pdpe1gb rdtscp lm constant_tsc rep_good amd_lbr_v2 nopl nonstop_tsc cpuid extd_apicid aperfmperf rapl pni pclmulqdq monitor ssse3 fma cx16 pcid sse4_1 sse4_2 x2apic movbe popcnt aes xsave avx f16c rdrand lahf_lm cmp_legacy svm extapic cr8_legacy abm sse4a misalignsse 3dnowprefetch osvw ibs skinit wdt tce topoext perfctr_core perfctr_nb bpext perfctr_llc mwaitx cpb cat_l3 cdp_l3 hw_pstate ssbd mba perfmon_v2 ibrs ibpb stibp ibrs_enhanced vmmcall fsgsbase bmi1 avx2 smep bmi2 erms invpcid cqm rdt_a avx512f avx512dq rdseed adx smap avx512ifma clflushopt clwb avx512cd sha_ni avx512bw avx512vl xsaveopt xsavec xgetbv1 xsaves cqm_llc cqm_occup_llc cqm_mbm_total cqm_mbm_local avx512_bf16 clzero irperf xsaveerptr rdpru wbnoinvd amd_ppin cppc arat npt lbrv svm_lock nrip_save tsc_scale vmcb_clean flushbyasid decodeassists pausefilter pfthreshold avic v_vmsave_vmload vgif x2avic v_spec_ctrl vnmi avx512vbmi umip pku ospke avx512_vbmi2 gfni vaes vpclmulqdq avx512_vnni avx512_bitalg avx512_vpopcntdq la57 rdpid overflow_recov succor smca fsrm flush_l1d debug_swap
Virtualization:                       AMD-V
L1d cache:                            6 MiB (192 instances)
L1i cache:                            6 MiB (192 instances)
L2 cache:                             192 MiB (192 instances)
L3 cache:                             768 MiB (24 instances)
NUMA node(s):                         2
NUMA node0 CPU(s):                    0-95,192-287
NUMA node1 CPU(s):                    96-191,288-383
Vulnerability Gather data sampling:   Not affected
Vulnerability Itlb multihit:          Not affected
Vulnerability L1tf:                   Not affected
Vulnerability Mds:                    Not affected
Vulnerability Meltdown:               Not affected
Vulnerability Mmio stale data:        Not affected
Vulnerability Reg file data sampling: Not affected
Vulnerability Retbleed:               Not affected
Vulnerability Spec rstack overflow:   Mitigation; Safe RET
Vulnerability Spec store bypass:      Mitigation; Speculative Store Bypass disabled via prctl
Vulnerability Spectre v1:             Mitigation; usercopy/swapgs barriers and __user pointer sanitization
Vulnerability Spectre v2:             Mitigation; Enhanced / Automatic IBRS; IBPB conditional; STIBP always-on; RSB filling; PBRSB-eIBRS Not affected; BHI Not affected
Vulnerability Srbds:                  Not affected
Vulnerability Tsx async abort:        Not affected

Removing debug pod ...
Temporary namespace openshift-debug-gh7zb was removed.
```
