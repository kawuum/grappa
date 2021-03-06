#!/usr/bin/env ruby
interactive = false
if ARGV.size > 1 && ARGV[0] == "--"
  interactive = true
end

## set up Google logging defaults
ENV["GLOG_logtostderr"] = "1"
ENV["GLOG_v"] = "0"

## set up GASNet debugging
ENV["GASNET_BACKTRACE"] = "1"
ENV["GASNET_FREEZE_SIGNAL"] = "SIGUSR1"
ENV["GASNET_FREEZE_ON_ERROR"] = "1"
ENV["GASNET_FREEZE"] = "0"
# ENV["GASNET_VERBOSEENV"] = "1"

case `hostname`
when /n\d+/ #(sampa)
  # workaround for 1G huge pages
  ENV["GASNET_PHYSMEM_MAX"] = "1024M"
end

## these will ensure we use RDMA rather than copying data to a bounce buffer
ENV["GASNET_PACKEDLONG_LIMIT"] = "0"
ENV["GASNET_NONBULKPUT_BOUNCE_LIMIT"] = "0"
ENV["GASNET_PUTINMOVE_LIMIT"] = "0"

## set Google profiler sample rate
ENV["CPUPROFILE_FREQUENCY"] = "50"

## set VampirTrace options
#ENV["VT_VERBOSE"] = "10"
ENV["VT_MAX_FLUSHES"] = "0"
ENV["VT_PFORM_GDIR"] = "."
ENV["VT_PFORM_LDIR"] = "/scratch"
ENV["VT_FILE_UNIQUE"] = "yes"
ENV["VT_MPITRACE"] = "no"

## set MVAPICH2 options to avoid keeping around malloced memory
## (and some performance tweaks which may be irrelevant)
ENV["MV2_USE_LAZY_MEM_UNREGISTER"] = "0"
ENV["MV2_ON_DEMAND_THRESHOLD"] = "3000"

ENV["MV2_DEFAULT_MAX_SEND_WQE"] = "2"
ENV["MV2_DEFAULT_MAX_RECV_WQE"] = "2"
ENV["MV2_MAX_INLINE_SIZE"] = "0"
ENV["MV2_NUM_RDMA_BUFFER"] = "4"

## set MVAPICH2 options to avoid keeping around malloced memory
ENV["OMPI_MCA_mpi_leave_pinned"] = "0"

## Clean up any leftover shared memory regions
s = `ipcs -m | grep $USER | awk '{print $2}' | xargs -n1 -r ipcrm -m`
s = `rm -f /dev/shm/GrappaLocaleSharedMemory`

if interactive
  exec ARGV[1..-1].join(' ')
end
