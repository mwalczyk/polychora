#pragma once

#include <algorithm>
#include <iostream>
#include <set>
#include <vector>

#include "epermute.h"

namespace four
{

	namespace combinatorics
	{

		enum class Parity
		{
			EVEN,
			ODD,
			ALL
		};

		/// Function to display the array 
		template<class T>
		void display(const std::vector<T>& values)
		{
			for (size_t i = 0; i < values.size(); i++)
			{
				std::cout << values[i];
				if (i != values.size() - 1)
				{
					std::cout << ", ";
				}
			}
			std::cout << std::endl;
		}

		/// Find all unique subsets of the given set
		///
		/// Reference: https://stackoverflow.com/questions/728972/finding-all-the-subsets-of-a-set
		template<typename T>
		std::vector<std::vector<T>> powerset(const std::vector<T>& set)
		{
			// Output
			std::vector<std::vector<T>> subsets;

			// If empty set, return set containing empty set
			if (set.empty())
			{
				subsets.push_back(set);

				return subsets;
			}

			// If only one element, return itself and empty set
			if (set.size() == 1)
			{
				std::vector<T> empty;
				subsets.push_back(empty);
				subsets.push_back(set);

				return subsets;
			}

			// Otherwise, get all but last element
			std::vector<T> all_but_last;
			for (size_t i = 0; i < (set.size() - 1); ++i)
			{
				all_but_last.push_back(set[i]);
			}

			// Get subsets of set formed by excluding the last element of the input set
			std::vector<std::vector<T> > subset_all_but_last = powerset(all_but_last);

			// First add these sets to the output
			for (size_t i = 0; i < subset_all_but_last.size(); ++i)
			{
				subsets.push_back(subset_all_but_last[i]);
			}
			// Now add to each set in ssallbutlast the last element of the input
			for (size_t i = 0; i < subset_all_but_last.size(); ++i)
			{
				subset_all_but_last[i].push_back(set[set.size() - 1]);
			}
			// Add these new sets to the output
			for (size_t i = 0; i < subset_all_but_last.size(); ++i)
			{
				subsets.push_back(subset_all_but_last[i]);
			}

			return subsets;
		}

		/// Reference: https://www.geeksforgeeks.org/all-permutations-of-an-array-using-stl-in-c/
		template<class T>
		std::vector<std::vector<T>> find_all_permutations(std::vector<T> values, Parity parity = Parity::ALL)
		{
			std::vector<std::vector<T>> permutations;

			// Sort the given array 
			std::sort(values.begin(), values.end());

			// Find all possible permutations that have the specified parity
			if (parity == Parity::ALL)
			{
				do
				{
					permutations.push_back(values);
				} while (next_permutation(values.begin(), values.end()));
			}
			else if (parity == Parity::EVEN)
			{
				do
				{
					permutations.push_back(values);
				} while (next_even_permutation(values.begin(), values.end()));
			}
			else
			{
				// TODO: ODD permutations
			}

			return permutations;
		}

		template<class T>
		struct PermutationSeed
		{
			std::vector<T> values;
			bool with_sign_changes;
			Parity parity;
		};

		template<class T>
		std::set<std::vector<T>> generate(const std::vector<PermutationSeed<T>>& permutation_seeds)
		{
			std::set<std::vector<T>> output_permutations;

			for (const auto& seed : permutation_seeds)
			{
				std::set<std::vector<T>> inputs;
				inputs.insert(seed.values);

				for (const auto& input : inputs)
				{
					// Calculate permutations for this subset of the inputs and update the output set
					auto current_permutations = find_all_permutations(input, seed.parity);
					for (const auto& permutation : current_permutations)
					{
						output_permutations.insert(permutation);

						// Do we also need to try all sign changes?
						if (seed.with_sign_changes)
						{
							// Generate a list of indices, i.e. 0, 1, 2, ..., values.size() - we will use these to 
							// "flip" the sign of a (sub)set of the elements below
							std::vector<size_t> indices(seed.values.size());
							size_t counter = 0;
							std::generate(indices.begin(), indices.end(), [&] { return counter++; });

							// Generate the powerset of the indices generated above - in other words, a set of all of the 
							// unique subsets (pairings) of the elements in the original list
							auto sign_changes = powerset(indices);

							for (const auto& sign_change : sign_changes)
							{
								// Make a copy of the original seed values and negate all of the relevant indices
								auto current_values = permutation;
								for (size_t index : sign_change)
								{
									current_values[index] *= -1;
								}
								output_permutations.insert(current_values);
							}
						}
					}
				}
			}

			return output_permutations;
		}
	}

}