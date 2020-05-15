
permutations = [
'(φ, 2φ2, 3+4φ, 3φ2)',
'(φ2, 2φ, 4+5φ, φ3)',
'(φ2, 2+φ, 3+4φ, 2φ3)',
'(φ2, φ3, 4φ2, φ4)',
'(φ2, φ4, 1+4φ, 2φ3)',
'(2φ, 1+3φ, 3+4φ, φ4)',
'(2+φ, φ3, φ5, 2φ2)',
'(φ3, 2φ2, 1+3φ, 2+5φ)',
'(0, 1, 4+5φ, 1+3φ)',
'(0, φ, φ5, 1+4φ)',
'(0, φ2, 3φ3, 2+φ)',
'(0, φ3, 2+5φ, 3φ2)',
'(1, φ2, 2+5φ, 2φ3)',
'(1, φ2, 4+5φ, 2φ2)',
'(1, 2φ, φ5, φ4)',
'(1, φ4, 2φ3, 3φ2)',
'(φ, φ2, 2φ, 3φ3)',
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

