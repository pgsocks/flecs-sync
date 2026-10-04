require 'open3'
require 'tmpdir'
require 'fileutils'
require 'rbconfig'
require 'timeout'

bundle, source = ARGV
Dir.mktmpdir('sync-runner-check-') do |directory|
	fixture = "#{directory}/fixture"
	File.write(fixture, <<~RUBY)
		#!#{RbConfig.ruby}
		STDOUT.sync = true
		mode = ENV.fetch('SYNC_RUNNER_MODE')
		while line = STDIN.gets
			case line.strip
			when /^begin / then puts 'ok ready=1'
			when /^step /
				exit 23 if mode == 'crash'
				value = mode == 'assertion' ? 0 : 1
				puts "ok operation=1 count=1 a0=\#{value} e0=1"
			when 'state' then puts 'ok operation=1 count=1 a0=1 e0=1'
			when 'quit'
				sleep 30 if mode == 'timeout'
				puts(mode == 'cleanup' ? 'ok closed=0' : 'ok closed=1')
				exit 0
			end
		end
	RUBY
	File.chmod(0o700, fixture)
	feature = "#{directory}/check.feature"
	%w[healthy assertion crash cleanup timeout undefined].each do |mode|
		body = if mode == 'undefined'
			'When a deliberately undefined step runs'
		else
			<<~FEATURE
				When I perform operation 1:
					"""
					query a real observation
					"""
				Then its observed values match the contract
			FEATURE
		end
		File.write(feature, <<~FEATURE)
			Feature: Runner failure detection
			Scenario: #{mode}
				Given the "core" workflow "check"
				#{body}
		FEATURE
		env = {
			'BUNDLE_GEMFILE' => "#{source}/Gemfile",
			'BUNDLE_FROZEN' => 'true', 'SYNC_FIXTURE' => fixture,
			'SYNC_RUNNER_MODE' => mode
		}
		output, status = Timeout.timeout(15) do
			Open3.capture2e(env, bundle, 'exec', 'cucumber',
				'--publish-quiet', '--strict', '--format', 'progress', '--require',
				"#{source}/features/support", '--require',
				"#{source}/features/step_definitions", feature)
		end
		passed = mode == 'healthy'
		raise "runner #{mode}: #{output}" unless status.success? == passed
		puts "Runner #{mode} behaved as expected"
	end
end
