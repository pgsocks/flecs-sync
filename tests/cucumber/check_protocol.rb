require 'open3'
require 'timeout'
require 'zlib'

# Exercise state retention, bad commands, actual values and complete teardown.
fixture, source = ARGV
feature = File.read("#{source}/features/core/module.feature")
scenario = feature.split('Scenario: module_world_isolation', 2).last
scenario = scenario.split(/^@core/, 2).first
pattern = Regexp.new('When I perform operation (\d+):\n' +
	'\s*"""\n(.*?)\n\s*"""', Regexp::MULTILINE)
operations = scenario.scan(pattern)
raise 'missing protocol workflow' if operations.empty?

Timeout.timeout(15) do
	Open3.popen3(fixture) do |input, output, error, process|
		diagnostics = Thread.new { error.read }
		rpc = lambda do |command|
			input.puts command
			input.flush
			line = output.gets
			raise "EOF for #{command}" unless line
			line.strip
		end
		raise 'bad begin accepted' unless rpc.call('garbage') ==
			'error expected=begin'
		raise 'begin failed' unless
			rpc.call('begin module_world_isolation') == 'ok ready=1'
		initial = rpc.call('state')
		raise 'initial state incorrect' unless
			initial == 'ok operation=0 count=0'
		%w[garbage step\ 99\ 0 step\ 1\ 0].each do |command|
			raise 'bad operation accepted' unless
				rpc.call(command).start_with?('error ')
			raise 'bad operation changed state' unless
				rpc.call('state') == initial
		end
		observations = []
		operations.each do |number, description|
			signature = Zlib.crc32(description.split.join(' '))
			line = rpc.call("step #{number} #{signature}")
			raise 'operation failed' unless line.start_with?('ok ')
			raise 'query mutated state' unless rpc.call('state') == line
			fields = line.split.drop(1).to_h { |f| f.split('=', 2) }
			raise 'operation sequence wrong' unless
				fields.fetch('operation') == number
			Integer(fields.fetch('count')).times do |i|
				a = fields.fetch("a#{i}")
				e = fields.fetch("e#{i}")
				raise "observed #{a}, expected #{e}" unless a == e
				observations << a
			end
		end
		raise 'role values were not queried' unless
			observations.include?('1') && observations.include?('2')
		raise 'teardown not acknowledged' unless
			rpc.call('quit') == 'ok closed=1'
		raise diagnostics.value unless process.value.success?
	end
end
puts 'Persistent protocol, observations and teardown passed'
