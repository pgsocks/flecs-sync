Given('the {string} workflow {string}') do |_suite, name|
	@last = rpc("begin #{name}")
	raise 'fixture not ready' unless @last.fetch('ready') == '1'
end

When('I perform operation {int}:') do |operation, description|
	raise 'missing procedural operation' if description.strip.empty?
	signature = Zlib.crc32(description.split.join(' '))
	@last = rpc("step #{operation} #{signature}")
	raise 'wrong fixture operation' unless
		@last.fetch('operation').to_i == operation
end

Then('its observed values match the contract') do
	count = Integer(@last.fetch('count'))
	count.times do |index|
		actual = @last.fetch("a#{index}")
		expected = @last.fetch("e#{index}")
		raise "observation #{index}: expected #{expected}, got #{actual}" if
			actual != expected
	end
	# Querying persistent state must not advance or alter an operation.
	raise 'state query changed observations' unless rpc('state') == @last
end

Then('the visible inventories are:') do |table|
	views = { 'Aria' => 0, 'Bram' => 1, 'GM' => 2 }
	actors = { 'Aria' => 0, 'Bram' => 1, 'Mira' => 2, 'Skulk' => 3 }
	items = {
		'HealingPotion' => 0, 'Arrow' => 1, 'IronSword' => 2,
		'GoldCoin' => 4
	}
	table.hashes.each do |row|
		view = views.fetch(row.fetch('View'))
		owner = actors.fetch(row.fetch('Owner'))
		item = items.fetch(row.fetch('Item'))
		quantity = row.fetch('Quantity')
		expected = quantity == 'hidden' ? '-1' : quantity
		actual = rpc("query quantity #{view} #{owner} #{item}")
		raise "inventory #{row}: #{actual}" unless
			actual.fetch('quantity') == expected
	end
end

Then('the example file result is {string}') do |result|
	results = {
		'SYNC_ERR_PROTOCOL' => '-6', 'SYNC_ERR_LIMIT' => '-9',
		'SYNC_ERR_TRUNCATED' => '-11', 'SYNC_ERR_UNSUPPORTED' => '-5',
		'SYNC_ERR_IO' => '-14', 'SYNC_ERR_NOMEM' => '-10'
	}
	actual = rpc('query result').fetch('result')
	raise "example expected #{result}, got #{actual}" unless
		actual == results.fetch(result)
end

Then('the example quest note is {string}') do |note|
	actual = rpc('query quest')
	expected_null = { 'null' => '1', 'empty' => '0' }.fetch(note)
	raise "quest expected #{note}, got #{actual}" unless
		actual.fetch('null') == expected_null && actual.fetch('length') == '0'
end

Then('the fixture example is {string}') do |example|
	actual = rpc('query example').fetch('example')
	raise "example expected #{example}, got #{actual}" unless actual == example
end
