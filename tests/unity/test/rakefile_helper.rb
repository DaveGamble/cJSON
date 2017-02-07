# ==========================================
#   Unity Project - A Test Framework for C
#   Copyright (c) 2007 Mike Karlesky, Mark VanderVoord, Greg Williams
#   [Released under MIT License. Please refer to license.txt for details]
# ==========================================

require 'yaml'
require 'fileutils'
require UNITY_ROOT + '../auto/unity_test_summary'
require UNITY_ROOT + '../auto/generate_test_runner'
require UNITY_ROOT + '../auto/colour_reporter'

module RakefileHelpers

  C_EXTENSION = '.c'

  def load_configuration(config_file)
    unless ($configured)
      $cfg_file = "targets/#{config_file}" unless (config_file =~ /[\\|\/]/)
      $cfg = YAML.load(File.read($cfg_file))
      $colour_output = false unless $cfg['colour']
      $configured = true if (config_file != DEFAULT_CONFIG_FILE)
    end
  end

  def configure_clean
    CLEAN.include($cfg['compiler']['build_path'] + '*.*') unless $cfg['compiler']['build_path'].nil?
  end

  def configure_toolchain(config_file=DEFAULT_CONFIG_FILE)
    config_file += '.yml' unless config_file =~ /\.yml$/
    config_file = config_file unless config_file =~ /[\\|\/]/
    load_configuration(config_file)
    configure_clean
  end

  def get_unit_test_files
    path = $cfg['compiler']['unit_tests_path'] + 'test*' + C_EXTENSION
    path.gsub!(/\\/, '/')
    FileList.new(path)
  end

  def get_local_include_dirs
    include_dirs = $cfg['compiler']['includes']['items'].dup
    include_dirs.delete_if {|dir| dir.is_a?(Array)}
    return include_dirs
  end

  def extract_headers(filename)
    includes = []
    lines = File.readlines(filename)
    lines.each do |line|
      m = line.match(/^\s*#include\s+\"\s*(.+\.[hH])\s*\"/)
      if not m.nil?
        includes << m[1]
      end
    end
    return includes
  end

  def find_source_file(header, paths)
    paths.each do |dir|
      src_file = dir + header.ext(C_EXTENSION)
      if (File.exists?(src_file))
        return src_file
      end
    end
    return nil
  end

  def tackit(strings)
    if strings.is_a?(Array)
      result = "\"#{strings.join}\""
    else
      result = strings
    end
    return result
  end

  def squash(prefix, items)
    result = ''
    items.each { |item| result += " #{prefix}#{tackit(item)}" }
    return result
  end

  def should(behave, &block)
    if block
      puts "Should " + behave
      yield block
    else
      puts "UNIMPLEMENTED CASE: Should #{behave}"
    end
  end

  def build_compiler_fields(inject_defines)
    command  = tackit($cfg['compiler']['path'])
    if $cfg['compiler']['defines']['items'].nil?
      defines  = ''
    else
      defines  = squash($cfg['compiler']['defines']['prefix'], $cfg['compiler']['defines']['items'] + ['UNITY_OUTPUT_CHAR=putcharSpy'] + inject_defines)
    end
    options  = squash('', $cfg['compiler']['options'])
    includes = squash($cfg['compiler']['includes']['prefix'], $cfg['compiler']['includes']['items'])
    includes = includes.gsub(/\\ /, ' ').gsub(/\\\"/, '"').gsub(/\\$/, '') # Remove trailing slashes (for IAR)
    return {:command => command, :defines => defines, :options => options, :includes => includes}
  end

  def compile(file, defines=[])
    compiler = build_compiler_fields(defines)
    defines  =
    cmd_str  = "#{compiler[:command]}#{compiler[:defines]}#{compiler[:options]}#{compiler[:includes]} #{file} " +
               "#{$cfg['compiler']['object_files']['prefix']}#{$cfg['compiler']['object_files']['destination']}"
    obj_file = "#{File.basename(file, C_EXTENSION)}#{$cfg['compiler']['object_files']['extension']}"
    execute(cmd_str + obj_file)
    return obj_file
  end

  def build_linker_fields
    command  = tackit($cfg['linker']['path'])
    if $cfg['linker']['options'].nil?
      options  = ''
    else
      options  = squash('', $cfg['linker']['options'])
    end
    if ($cfg['linker']['includes'].nil? || $cfg['linker']['includes']['items'].nil?)
      includes = ''
    else
      includes = squash($cfg['linker']['includes']['prefix'], $cfg['linker']['includes']['items'])
    end
    includes = includes.gsub(/\\ /, ' ').gsub(/\\\"/, '"').gsub(/\\$/, '') # Remove trailing slashes (for IAR)
    return {:command => command, :options => options, :includes => includes}
  end

  def link_it(exe_name, obj_list)
    linker = build_linker_fields
    cmd_str = "#{linker[:command]}#{linker[:options]}#{linker[:includes]} " +
      (obj_list.map{|obj|"#{$cfg['linker']['object_files']['path']}#{obj} "}).join +
      $cfg['linker']['bin_files']['prefix'] + ' ' +
      $cfg['linker']['bin_files']['destination'] +
      exe_name + $cfg['linker']['bin_files']['extension']
    execute(cmd_str)
  end

  def build_simulator_fields
    return nil if $cfg['simulator'].nil?
    if $cfg['simulator']['path'].nil?
      command = ''
    else
      command = (tackit($cfg['simulator']['path']) + ' ')
    end
    if $cfg['simulator']['pre_support'].nil?
      pre_support = ''
    else
      pre_support = squash('', $cfg['simulator']['pre_support'])
    end
    if $cfg['simulator']['post_support'].nil?
      post_support = ''
    else
      post_support = squash('', $cfg['simulator']['post_support'])
    end
    return {:command => command, :pre_support => pre_support, :post_support => post_support}
  end

  def execute(command_string, ok_to_fail=false)
    report command_string if $verbose
    output = `#{command_string}`.chomp
    report(output) if ($verbose && !output.nil? && (output.length > 0))
    if (($?.exitstatus != 0) && !ok_to_fail)
      raise "Command failed. (Returned #{$?.exitstatus})"
    end
    return output
  end

  def report_summary
    summary = UnityTestSummary.new
    summary.set_root_path(UNITY_ROOT)
    results_glob = "#{$cfg['compiler']['build_path']}*.test*"
    results_glob.gsub!(/\\/, '/')
    results = Dir[results_glob]
    summary.set_targets(results)
    report summary.run
  end

  def run_tests(test_files)
    report 'Running Unity system tests...'

    # Tack on TEST define for compiling unit tests
    load_configuration($cfg_file)
    test_defines = ['TEST']
    $cfg['compiler']['defines']['items'] = [] if $cfg['compiler']['defines']['items'].nil?
    $cfg['compiler']['defines']['items'] << 'TEST'

    include_dirs = get_local_include_dirs

    # Build and execute each unit test
    test_files.each do |test|
      obj_list = []

      if !$cfg['compiler']['aux_sources'].nil?
        $cfg['compiler']['aux_sources'].each do |aux|
          obj_list << compile(aux, test_defines)
        end
      end

      # Detect dependencies and build required modules
      extract_headers(test).each do |header|
        # Compile corresponding source file if it exists
        src_file = find_source_file(header, include_dirs)
        if !src_file.nil?
          obj_list << compile(src_file, test_defines)
        end
      end

      # Build the test runner (generate if configured to do so)
      test_base = File.basename(test, C_EXTENSION)

      runner_name = test_base + '_Runner.c'
      runner_path = ''

      if $cfg['compiler']['runner_path'].nil?
        runner_path = $cfg['compiler']['build_path'] + runner_name
      else
        runner_path = $cfg['compiler']['runner_path'] + runner_name
      end

      options = $cfg[:unity]
      options[:use_param_tests] = (test =~ /parameterized/) ? true : false
      UnityTestRunnerGenerator.new(options).run(test, runner_path)
      obj_list << compile(runner_path, test_defines)

      # Build the test module
      obj_list << compile(test, test_defines)

      # Link the test executable
      link_it(test_base, obj_list)

      # Execute unit test and generate results file
      simulator = build_simulator_fields
      executable = $cfg['linker']['bin_files']['destination'] + test_base + $cfg['linker']['bin_files']['extension']
      if simulator.nil?
        cmd_str = executable
      else
        cmd_str = "#{simulator[:command]} #{simulator[:pre_support]} #{executable} #{simulator[:post_support]}"
      end
      output = execute(cmd_str)
      test_results = $cfg['compiler']['build_path'] + test_base
      if output.match(/OK$/m).nil?
        test_results += '.testfail'
      else
        report output if (!$verbose) #verbose already prints this line, as does a failure
        test_results += '.testpass'
      end
      File.open(test_results, 'w') { |f| f.print output }

    end
  end
end
