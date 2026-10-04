require 'open3'
require 'timeout'
require 'tmpdir'
require 'fileutils'
require 'zlib'

Before do
	@directory = Dir.mktmpdir('sync-cucumber-')
	@input, @output, @error, @process =
		Open3.popen3(ENV.fetch('SYNC_FIXTURE'), chdir: @directory)
	@stderr = Thread.new { @error.read }
end

After do
	begin
		if @process
			raise "fixture exited before cleanup: #{fixture_diagnostics}" unless
				@process.alive?
			reply = rpc('quit')
			raise 'fixture cleanup was not acknowledged' unless
				reply.fetch('closed') == '1'
			status = Timeout.timeout(5) { @process.value }
			raise "fixture exited #{status}: #{fixture_diagnostics}" unless
				status.success?
		end
	rescue Timeout::Error
		warn 'fixture cleanup timed out'
		raise 'fixture cleanup timed out'
	rescue StandardError => error
		warn error.message
		raise
	ensure
		if @process&.alive?
			Process.kill('KILL', @process.pid)
			@process.join(5)
		end
		[@input, @output].compact.each { |io| io.close unless io.closed? }
		@stderr&.join(5)
		@error&.close
		FileUtils.remove_entry(@directory) if @directory
	end
end

def rpc(command)
	Timeout.timeout(5) do
		@input.puts command
		@input.flush
		line = @output.gets
		raise "fixture EOF for #{command}: #{fixture_diagnostics}" unless line
		raise "fixture rejected #{command}: #{line}" unless
			line.start_with?('ok ')
		line.strip.split.drop(1).to_h do |field|
			field.split('=', 2)
		end
	end
rescue Timeout::Error
	raise "fixture command timed out: #{command}"
end

def fixture_diagnostics
	return '' unless @stderr
	return 'fixture stderr is still open' unless @stderr.join(1)
	@stderr.value
end
