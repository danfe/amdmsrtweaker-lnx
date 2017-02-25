/*
 * Copyright (c) Martin Kinkelin
 *
 * See the "License.txt" file in the root directory for infos
 * about permitted and prohibited uses of this code.
 */

#include <cstdio>
#include <cstdlib>

#if defined(__FreeBSD__)
#include <sys/types.h>
#include <sys/cpuctl.h>
#include <sys/pciio.h>
#include <err.h>
#define _PATH_DEVPCI "/dev/pci"
#define _PATH_DEVCPUCTL "/dev/cpuctl" // index is appended in situ
#endif
#include <fcntl.h>
#include <unistd.h>

#include "WinRing0.h"
#include "StringUtils.h"

using std::exception;
using std::string;

static int get_num_cpu() {
    CpuidRegs regs = Cpuid(0x80000008);
    return 1 + (regs.ecx & 0xff);
}

#if defined(__linux__)
uint32_t ReadPciConfig(uint32_t device, uint32_t function, uint32_t regAddress) {
    uint32_t result;
    char path[32];
    snprintf(path, sizeof(path), "/proc/bus/pci/00/%02x.%x", device, function);

    int pci = open(path, O_RDONLY);
    if (pci == -1) {
        perror("Failed to open pci device for reading");
        exit(-1);
    }
    pread(pci, &result, sizeof(result), regAddress);
    close(pci);

    return result;
}

void WritePciConfig(uint32_t device, uint32_t function, uint32_t regAddress, uint32_t value) {
    char path[32];
    snprintf(path, sizeof(path), "/proc/bus/pci/00/%02x.%x", device, function);

    int pci = open(path, O_WRONLY);
    if (pci == -1) {
        perror("Failed to open pci device for writing");
        exit(-1);
    }
    if(pwrite(pci, &value, sizeof(value), regAddress) != sizeof(value)) {
        perror("Failed to write to pci device");
    }
    close(pci);
}


uint64_t Rdmsr(uint32_t index) {
    uint64_t result;

    int msr = open("/dev/cpu/0/msr", O_RDONLY);
    if (msr == -1) {
        perror("Failed to open msr device for reading");
        exit(-1);
    }
    pread(msr, &result, sizeof(result), index);
    close(msr);

    return result;
}

void Wrmsr(uint32_t index, const uint64_t& value) {
    char path[32];
    int ncpu;

    ncpu = get_num_cpu();
    for (int i = 0; i < ncpu; i++) {
        snprintf(path, sizeof(path), "/dev/cpu/%u/msr", i);
        int msr = open(path, O_WRONLY);
        if (msr == -1) {
            perror("Failed to open msr device for writing");
            exit(-1);
        }
        if(pwrite(msr, &value, sizeof(value), index) != sizeof(value)) {
            perror("Failed to write to msr device");
        }
        close(msr);
    }
}


CpuidRegs Cpuid(uint32_t index) {
    CpuidRegs result;

    FILE* cpuid = fopen("/dev/cpu/0/cpuid", "r");
    if (cpuid == NULL) {
        perror("Failed to open cpuid device for reading");
        exit(-1);
    }
    fseek(cpuid, index, SEEK_SET);
    fread(&(result.eax), sizeof(result.eax), 1, cpuid);
    fread(&(result.ebx), sizeof(result.ebx), 1, cpuid);
    fread(&(result.ecx), sizeof(result.ecx), 1, cpuid);
    fread(&(result.edx), sizeof(result.edx), 1, cpuid);
    fclose(cpuid);

    return result;
}
#elif defined(__FreeBSD__)
uint32_t ReadPciConfig(uint32_t device, uint32_t function, uint32_t regAddress) {
    struct pci_io pi;
    int fd;

    fd = open(_PATH_DEVPCI, O_RDWR);
    if (fd < 0)
        err(-1, "Failed to open %s", _PATH_DEVPCI);

    pi.pi_sel.pc_domain = pi.pi_sel.pc_bus = 0;
    pi.pi_sel.pc_dev = device;
    pi.pi_sel.pc_func = function;
    pi.pi_reg = regAddress;
    pi.pi_width = sizeof(uint32_t);

    if (ioctl(fd, PCIOCREAD, &pi) < 0)
        err(-2, "ioctl(PCIOCREAD) on %s", _PATH_DEVPCI);
    close(fd);

    return (pi.pi_data);
}

void WritePciConfig(uint32_t device, uint32_t function, uint32_t regAddress, uint32_t value) {
    struct pci_io pi;
    int fd;

    fd = open(_PATH_DEVPCI, O_RDWR);
    if (fd < 0)
        err(-1, "Failed to open %s", _PATH_DEVPCI);

    pi.pi_sel.pc_domain = pi.pi_sel.pc_bus = 0;
    pi.pi_sel.pc_dev = device;
    pi.pi_sel.pc_func = function;
    pi.pi_reg = regAddress;
    pi.pi_width = sizeof(uint32_t);
    pi.pi_data = value;

    if (ioctl(fd, PCIOCWRITE, &pi) < 0)
        err(-2, "ioctl(PCIOCWRITE) on %s", _PATH_DEVPCI);
    close(fd);
}

uint64_t Rdmsr(uint32_t index) {
    cpuctl_msr_args_t cma;
    int fd;

    fd = open(_PATH_DEVCPUCTL "0", O_RDWR);
    if (fd < 0)
        err(-1, "Failed to open %s", _PATH_DEVCPUCTL "0");

    cma.msr = index;

    if (ioctl(fd, CPUCTL_RDMSR, &cma) < 0)
        err(-2, "ioctl(CPUCTL_RDMSR) on %s", _PATH_DEVCPUCTL "0");
    close(fd);

    return cma.data;
}

void Wrmsr(uint32_t index, const uint64_t& value) {
    char path[32];
    cpuctl_msr_args_t cma;
    int fd, ncpu;

    cma.msr = index;
    cma.data = value;

    ncpu = get_num_cpu();
    for (int i = 0; i < ncpu; i++) {
        snprintf(path, sizeof(path), _PATH_DEVCPUCTL "%d", i);
        fd = open(path, O_RDWR);
        if (fd < 0)
            err(-1, "Failed to open %s", path);
        if (ioctl(fd, CPUCTL_WRMSR, &cma) < 0)
            warn("ioctl(CPUCTL_WRMSR) on %s", path);
        close(fd);
    }
}

CpuidRegs Cpuid(uint32_t index) {
    CpuidRegs result;
    cpuctl_cpuid_args_t cca;
    int fd;

    fd = open(_PATH_DEVCPUCTL "0", O_RDWR);
    if (fd < 0)
        err(-1, "Failed to open %s", _PATH_DEVCPUCTL "0");

    cca.level = index;

    if (ioctl(fd, CPUCTL_CPUID, &cca) < 0)
        err(-2, "ioctl(CPUCTL_CPUID) on %s", _PATH_DEVCPUCTL "0");
    close(fd);

    // XXX: would memcpy()/bcopy() suffice here?
    result.eax = cca.data[0];
    result.ebx = cca.data[1];
    result.ecx = cca.data[2];
    result.edx = cca.data[3];

    return result;
}
#else
#error >>> unsupported operating system <<<
#endif
