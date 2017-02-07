HERE = File.expand_path(File.dirname(__FILE__)) + '/'
UNITY_ROOT = File.expand_path(File.dirname(__FILE__)) + '/../..'

require 'rake'
require 'rake/clean'
require HERE+'rakefile_helper'

TEMP_DIRS = [
	File.join(HERE, 'build')
]

TEMP_DIRS.each do |dir|
  directory(dir)
  CLOBBER.include(dir)
end

task :prepare_for_tests => TEMP_DIRS

include RakefileHelpers

# Load default configuration, for now
DEFAULT_CONFIG_FILE = 'target_gcc_32.yml'
configure_toolchain(DEFAULT_CONFIG_FILE)

task :unit => [:prepare_for_tests] do
  run_tests get_unit_test_files
end

desc "Generate test summary"
task :summary do
  report_summary
end

desc "Build and test Unity"
task :all => [:clean, :unit, :summary]
task :default => [:clobber, :all]
task :ci => [:default]
task :cruise => [:default]

desc "Load configuration"
task :config, :config_file do |t, args|
  configure_toolchain(args[:config_file])
end
