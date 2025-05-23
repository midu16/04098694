# 04095854 - thread_load  


1. How to run the podman
```bash
podman run -it --privileged --pid=host quay.io/midu/thread-test:latest
```

2. Run the support-tools-with-perf container in a distinct terminal
```bash
podman run -it --privileged --pid=host quay.io/midu/support-tools-with-perf:latest bash
``` 

3. In the `support-tools-with-perf` container run the following

```bash
[root@4681694987cf /]# ps -eo pid,ppid,cmd,%cpu,%mem --sort=-%cpu | head
    PID    PPID CMD                         %CPU %MEM
2004566 2004565 ./thread_load -b 100 -m 100 4656  0.0
   9631       1 /usr/bin/kubelet --config=/ 27.7  0.0
  11765   11700 /bin/node_exporter --web.li  3.2  0.0
   6197       1 ovs-vswitchd unix:/var/run/  3.1  0.1
   9574       1 /usr/bin/crio                2.6  0.0
  13177   13163 /usr/bin/ovnkube --init-ovn  2.2  0.0
  10383   10355 dynkeepalived /var/lib/kube  0.8  0.0
   6035       1 /usr/sbin/irqbalance --fore  0.6  0.0
2004526 2004525 /bin/crictl inspectp f8f9e7  0.6  0.0
```

Profile the `thread_load` process:
```bash
[root@4681694987cf /]# perf record -p $(ps -eo pid,%cpu --sort=-%cpu | head -n 2 | tail -n 1 | cut -d ' ' -f1) -g -- sleep 30 
[ perf record: Woken up 10 times to write data ]
[ perf record: Captured and wrote 347.638 MB perf.data (2506514 samples) ]
Terminated
```

> ** NOTE: ** Step 3 should be executed while on another terminal the following command its being executed:

```bash
$ sudo ./thread_load -b 100 -m 100 -i 90 -d 10 -o 10000000 -t 1 -l 10
[16:21:27] [Parent] Using THREAD_BATCH_INTERVAL_SEC = 90, THREAD_BATCH = 100, MAX_THREADS = 100, THREADS_PER_MUTEX = 1, LOOP_COUNT = 10, THREAD_SLEEP_US = 10, OPERATIONS = 10000000
[16:21:27] [Parent] Forked child process 2017174
[16:21:27] [Parent] All processes and threads launched. Parent entering infinite wait mode.
[16:21:27] [Child] Process 2017174 started
[16:21:27] [Child] Created 100 threads, batch starting at 0
[16:21:39] operation finished - 10000000
```

4. Analyze the saved performance data:

```bash
[root@4681694987cf /]# perf report -i perf.data 
```

5. Copy the performance data outside of the `support-tools-with-perf` container:

> ** NOTE: ** The `support-tools-with-perf` [Containerfile](./support-tools/Containerfile)

```bash
[root@cnfdc13 ~]# podman ps 
CONTAINER ID  IMAGE                                        COMMAND     CREATED            STATUS            PORTS       NAMES
4681694987cf  quay.io/midu/support-tools-with-perf:latest  bash        About an hour ago  Up About an hour              relaxed_noyce
[root@cnfdc13 ~]# podman cp relaxed_noyce:/perf.data /var/home/core/midu/perf-5.14.0-427.58.1.el9_4.x86_64.data
[root@cnfdc13 ~]# ls -l /var/home/core/midu/perf-5.14.0-427.58.1.el9_4.x86_64.data
-rw-------. 1 root root 365020538 May 17 16:21 /var/home/core/midu/perf-5.14.0-427.58.1.el9_4.x86_64.data
```

6. Testing coverage

> ** NOTE: ** The performance tests were covering the OpenShift 4.16.37 following kernels:
> - kernel-5.14.0-427.57.1.el9_4.x86_64 - the OpenShift 4.16.37 default kernel
> - [kernel-5.14.0-427.58.1.el9_4.x86_64](https://github.com/midu16/rhcos-layering/blob/main/kernel-5.14.0-427.58.1.el9_4.x86_64/99-kernel-5.14.0-427.58.1.el9_4.x86_64.yaml)
> - [kernel-5.14.0-427.59.1.el9_4.x86_64](https://github.com/midu16/rhcos-layering/blob/main/kernel-5.14.0-427.59.1.el9_4.x86_64/99-kernel-5.14.0-427.59.1.el9_4.x86_64.yaml)
> - [kernel-5.14.0-427.60.1.el9_4.x86_64](https://github.com/midu16/rhcos-layering/blob/main/kernel-5.14.0-427.59.1.el9_4.x86_64/99-kernel-5.14.0-427.60.1.el9_4.x86_64.yaml)
> - [kernel-5.14.0-427.61.1.el9_4.x86_64](https://github.com/midu16/rhcos-layering/blob/main/kernel-5.14.0-427.59.1.el9_4.x86_64/99-kernel-5.14.0-427.61.1.el9_4.x86_64.yaml)
> - [kernel-5.14.0-427.62.1.el9_4.x86_64] - the Openshift 4.16.38 default kernel
> - [kernel-5.14.0-427.63.1.el9_4.x86_64](https://github.com/midu16/rhcos-layering/blob/main/kernel-5.14.0-427.63.1.el9_4.x86_64/99-kernel-5.14.0-427.63.1.el9_4.x86_64.yaml)
> - [kernel-5.14.0-427.64.1.el9_4.x86_64](https://github.com/midu16/rhcos-layering/blob/main/kernel-5.14.0-427.64.1.el9_4.x86_64/99-kernel-5.14.0-427.64.1.el9_4.x86_64.yaml)
> - [kernel-5.14.0-570.16.1.el9_6.x86_64](https://github.com/midu16/rhcos-layering/blob/main/kernel-5.14.0-570.16.1.el9_6.x86_64/99-kernel-5.14.0-570.16.1.el9_6.x86_64.yaml)
