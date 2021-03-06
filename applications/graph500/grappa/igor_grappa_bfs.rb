#!/usr/bin/env ruby
require 'igor'

# inherit parser, sbatch_flags
require_relative '../../../util/igor_common.rb'

Igor do
  include Isolatable
  
  database '~/exp/pgas.sqlite', :bfs
  
  # isolate everything needed for the executable so we can sbcast them for local execution
  isolate(['graph.exe'],
    File.dirname(__FILE__))
  
  GFLAGS.merge!({
    flat_combining: 1,
    beamer_alpha: 20,
    beamer_beta: 20,
  })
  
  params.merge!(GFLAGS)
  
  @c = ->{ %Q[ %{tdir}/grappa_srun
    -- %{tdir}/graph.exe
    #{GFLAGS.expand}
    -- -s %{scale} -e %{edgefactor} -f %{nbfs}
  ].gsub(/\s+/,' ') }
  command @c[]
  
  sbatch_flags << "--time=4:00:00"
  
  params {
    nnode       2
    ppn         1
    scale       16
    edgefactor  16
    nbfs        8
    fc_version 'twolevel<1024>'
  }
  
  expect :max_teps
  
  $filtered = results{|t| t.select(:id, :mpibfs, :scale, :nnode, :ppn, :run_at, :min_time, :max_teps) }
    
  interact # enter interactive mode
end
