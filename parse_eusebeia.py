
permutations = [
'(0, 0, 4φ, 2φ4)',
'(2+φ, 2+φ, 2+5φ, 2+5φ)',
]
for permutation in permutations:

	prefix = 'four::combinatorics::PermutationSeed<float>{ '
	postfix = ', true, four::combinatorics::Parity::ALL },'

	formatted = ''
	for i in range(len(permutation)):

		if permutation[i] == '(':
			formatted += '{'
		elif permutation[i] == ')':
			formatted += '}'
		elif permutation[i] == 'φ':
			if permutation[i + 1].isdigit():
				formatted += 'phi_'
			else:
				formatted += 'phi_1'
		elif permutation[i].isdigit():
			formatted += permutation[i]
			if permutation[i + 1] == 'φ':
				formatted += '*'
		else:
			formatted += permutation[i]

	print(prefix + formatted + postfix)

