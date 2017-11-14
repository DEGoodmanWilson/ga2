// ga2 - C++ genetic algorithm library
// Copyright (C) 2001 Donald E. Goodman
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
//
// ga2Population.cpp: implementation of the ga2Population class.
//
//////////////////////////////////////////////////////////////////////

#include <fstream>
#include <time.h>
#include <math.h>
#include "ga2.h"

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

/**
 * \param initialSize The initial population size
 * \param chromoSize Number of genes per chromosome
 *
 * Constructs a population of chromosomes, pre-allocating each.
 */
ga2Population::ga2Population( int initialSize, int chromoSize ) : _size(initialSize),
																 _chromoSize(chromoSize)
{
	//now initialize random number generator
	srand(time(NULL));
	_integer = false;
	_isSorted = false;
	_chromosomes.reserve(2*initialSize);
	_nextGen.reserve(initialSize);
}

/**
 * Destructor. Duh.
 */
ga2Population::~ga2Population()
{
	if(!_chromosomes.empty())
		_chromosomes.clear();
	if(!_nextGen.empty())
		_nextGen.clear();
	if(!_chromoMaxRanges.empty())
		_chromoMaxRanges.clear();
	if(!_chromoMinRanges.empty())
		_chromoMinRanges.clear();
}

/**
 * This function does a lot of initialisation that could not or should not
 * be done in the constructor for various reasons. It should be called after
 * setting the min (ga2Population::setMinRanges()) and max ranges
 * (ga2Population::setMaxRanges()) and setting the fitness function
 * (ga2Population::setEvalFunc()). It calls ga2Chromosome::randomInit(), and
 * evaluates ALL of the chromosomes! If the population is to be sorted by
 * fitness (only appropriate for certain types of selection. See
 * ga2Population::setSelectType()), ga2Population::setSort() should be called
 * first.
 */
bool ga2Population::init(void)
{
	//_chromoSize = chromoSize;
	if( (_chromoMaxRanges.size() != _chromoSize)
	  ||(_chromoMinRanges.size() != _chromoSize) )
		return false;

	int i;
	for(i = 0; i < _size; ++i)
	{
		ga2Chromosome newChromo(_chromoSize);
		newChromo.setMaxRanges(_chromoMaxRanges);
		newChromo.setMinRanges(_chromoMinRanges);
		newChromo.randomInit(_integer);
		newChromo.setEvalFunc(_evalFunc);
		newChromo.evaluate();
		if(!_isSorted)
			_chromosomes.push_back(newChromo);
		else //we have to do an insertion sort. large elements first, small last
		{
			if(_chromosomes.empty()) //if theres nothing there, just stuff it in there
				_chromosomes.push_back(newChromo);
			else //we have to look for the right place to put it...
			{
				//get an iterator
				std::vector<ga2Chromosome>::iterator it = _chromosomes.begin();
				//loop until we find the insertion point
				while((it != _chromosomes.end()) &&
					  (it->getFitness() > newChromo.getFitness()) )
					it++;
				_chromosomes.insert(it, newChromo);
			}
		}
	}
	return true;
}

/**
 * The selection function. Call when you are ready to select parents for
 * the next generation.
 */
bool ga2Population::select(void)
{
	//select pairs of chromosomes and put them in nextGen
	_nextGen.clear();
	int i, s1, s2;
	for(i = 0; i < _replacementSize; i+=2)
	{
		s1 = _selectFunc();
		s2 = _selectFunc();
		
		_chromosomes[s1].setParent(0, s1);
		_chromosomes[s1].setParent(1, s1);
		_chromosomes[s2].setParent(0, s2);
		_chromosomes[s2].setParent(1, s2);
		_nextGen.push_back(_chromosomes[s1]);
		_nextGen.push_back(_chromosomes[s2]);
	}
	return true;
}

/**
 * Calls the fitness function assigned to each chromosome, if necessary.
 * If a chromosome has already been evaluated, and has not changed since,
 * it is not evaluated again.
 */
bool ga2Population::evaluate(void)
{
	int i;
	float f;
	_sumFitness = _avgFitness = 0.0;
	_maxFitness = -1*(double)INT_MAX;
	_minFitness = (double)INT_MAX;
	for(i = 0; i < _size; ++i)
	{
		f = _chromosomes[i].getFitness();
		_sumFitness += f;
		if(f > _maxFitness) _maxFitness = f;
		if(f < _minFitness) _minFitness = f;
	}
	_avgFitness = _sumFitness/(double)_size;
	return true;
}

/**
 * Perform crossover after selecting new parents. The next generation is
 * stored for replacement, and can be mutated.
 */
bool ga2Population::crossover(void)
{
	_crossCount = 0;
	int i;
	for(i = 0; i < _replacementSize; i+=2)
	{
		_crossoverFunc(_nextGen[i], _nextGen[i+1]);
	}
	return true;
}

/**
 * Mutate the next generation, created by selecting and crossover.
 */
bool ga2Population::mutate(void)
{
	_mutationCount = 0;
	int i;
	for(i = 0; i < _replacementSize; ++i)
	{
		_mutateFunc(_nextGen[i]);
	}
	return true;
}

/**
 * Replace the current generation with the next generation.
 */
bool ga2Population::replace(void)
{
	return _replaceFunc();
}

int ga2Population::_selectRoulette(void)
{
	float partialSum= 0.0, sumFitness = 0.0;
	float wheelPosition = 0.0;
	int i;
	
	//initialize some stuff, like the total fitness of the population
	for(i = 0; i < _size; ++i)
	{
		sumFitness += _chromosomes[i].getFitness();
	}

	//spin that wheel!
	wheelPosition = ((float)rand()/(float)RAND_MAX) * sumFitness;

	i = -1;
	do
	{
		++i;
		partialSum += _chromosomes[i].getFitness();
	}while( (partialSum < wheelPosition) && (i != _size-1) );

	return _size-i;
}

int ga2Population::_selectRanked(void)
{
	float partialSum= 0.0, sumFitness = 0.0;
	float wheelPosition = 0.0;
	int i;
	
	//initialize some stuff, like the total fitness of the population
	for(i = 0; i < _size; ++i)
	{
		//instead of fitness, use rank!
		sumFitness += _size - i;
	}

	//spin that wheel!
	wheelPosition = ((float)rand()/(float)RAND_MAX) * sumFitness;

	i = 0;
	while( (i != _size-1) && (partialSum < wheelPosition) )
	{
		//again, use rank, not fitness
		++i;
		partialSum += i;
	}
	return i;
}

bool ga2Population::_crossoverOnePoint(ga2Chromosome &a, ga2Chromosome &b)
{
	float coPoint = ((float)rand()/(float)RAND_MAX) * _chromoSize;
	ga2Chromosome *a1, *a2, *b1, *b2;
	a1 = a.grabSlice(0, coPoint);
	a2 = a.grabSlice(coPoint, _chromoSize);
	b1 = b.grabSlice(0, coPoint);
	b2 = b.grabSlice(coPoint, _chromoSize);

	ga2Chromosome aNew(_chromoSize), bNew(_chromoSize);
	
	aNew = *b2 + *a1;
	bNew = *a2 + *b1;
	int tempParent1, tempParent2;
	tempParent1 = a.getParent(0);
	tempParent2 = b.getParent(0);

	delete(&a);
	delete(&b);
	
	a = aNew;
	b = bNew;
	
	a.setCrossSite(coPoint);
	a.setParent(0, tempParent1);
	a.setParent(1, tempParent2);
	b.setCrossSite(coPoint);
	b.setParent(0, tempParent1);
	b.setParent(1, tempParent2);

	return true;
}

bool ga2Population::_crossoverUniform(ga2Chromosome &a, ga2Chromosome &b)
{
	ga2Chromosome *c, *d;
	int flip;
	int i;
	
	//where does the first gene come from? after that, we loop.
	flip = ((float)rand()/(float)RAND_MAX) * 2;
	if(flip)
	{
		c = a.grabSlice(0, 1);
		d = b.grabSlice(0, 1);
	}
	else
	{
		c = b.grabSlice(0, 1);
		d = a.grabSlice(0, 1);
	}
	//now, loop and deal with the rest.
	for(i = 1; i < _chromoSize; ++i)
	{
		flip = ((float)rand()/(float)RAND_MAX) * 2;
		if(flip)
		{
			*c = *c + *(a.grabSlice(i, i+1));
			*d = *d + *(b.grabSlice(i, i+1));
		}
		else
		{
			*c = *c + *(b.grabSlice(i, i+1));
			*d = *d + *(a.grabSlice(i, i+1));
		}
	}

	//ok ,here is where things get weird. cause we gotta grab a and b's
	//parents (as set by the selection function) and put them first into c and d
	//before we can reassign a and b to be c and d.
	int tempParent1, tempParent2;
	tempParent1 = a.getParent(0);
	tempParent2 = b.getParent(0);

	//the old a and b need to be dealt with somehow...geez, this is icky.
	//delete(&a);
	//delete(&b);
	
	a = *c;
	b = *d;

	a.setCrossSite(0); //er, no real cross site to speak of in uniform...
	a.setParent(0, tempParent1);
	a.setParent(1, tempParent2);
	b.setCrossSite(0);
	b.setParent(0, tempParent1);
	b.setParent(1, tempParent2);

	return true;
}

int ga2Population::_selectFunc(void)
{
	switch(_selectionType)
	{
		case GA2_SELECT_RANKED:
			return _selectRanked();
			break;
		case GA2_SELECT_ROULETTE:
		default:
			return _selectRoulette();
	}
}

bool ga2Population::_replaceFunc(void)
{
	switch(_replacementType)
	{
		case GA2_REPLACE_STEADYSTATENODUPLICATES:
			return _replaceSteadyStateNoDuplicates();
			break;
		case GA2_REPLACE_STEADYSTATE:
			return _replaceSteadyState();
			break;
		case GA2_REPLACE_GENERATIONAL:
		default:
			return _replaceGenerational();
	}

}

bool ga2Population::_crossoverFunc(ga2Chromosome &a, ga2Chromosome &b)
{
	if( _crossoverRate != 1.0)
	{
		//need a random number to compare against crossover rate
		float probability = (float)rand() / (float)RAND_MAX;
		if(probability > _crossoverRate)
			return false;
	}

	++_crossCount;

	switch(_crossoverType)
	{
		case GA2_CROSSOVER_UNIFORM:
			return _crossoverUniform(a, b);
			break;
		case GA2_CROSSOVER_ONEPOINT:
		default:
			return _crossoverOnePoint(a, b);
	}
}

bool ga2Population::_mutateFunc(ga2Chromosome &a)
{
	int i;
	for(i = 0; i < _chromoSize; ++i)
	{
		float probability = (float)rand()/(float)RAND_MAX;
		if(probability <= _mutationRate)
		{
			//we mutate!
			++_mutationCount;
			float range = _chromoMaxRanges[i] - _chromoMinRanges[i];
			float f;
			if(_integer)
			{
				f = (((float)rand()/(float)RAND_MAX) * (range+1)) + _chromoMinRanges[i];
				f = (int)f; //trunc it down to size
			}
			else //no rounding
				f = (((float)rand()/(float)RAND_MAX) * range) + _chromoMinRanges[i];

				f = (((float)rand()/(float)RAND_MAX) * range) + _chromoMinRanges[i];
			a.setGene(i, f);
		}
	}
	return true;
}

//IMPORTANT! ACHTUNG!
//note on this function:
//it will not work (it will, in fact, return false without doing anything)
//if _isSorted is false!! ie, it only works on sorted populations!!
bool ga2Population::_replaceSteadyState(void)
{
	int i;
	if(!_isSorted)
		return false;

	//note that we dont care about the replacement size
	for(i = 0; i < _nextGen.size(); ++i)
	{
		//we have to do an insertion sort. large elements first, small last
		if(_chromosomes.empty()) //if theres nothing there, just stuff it in there
			_chromosomes.push_back(_nextGen[i]);
		else //we have to look for the right place to put it...
		{
			//get an iterator
			std::vector<ga2Chromosome>::iterator it = _chromosomes.begin();
			//loop until we find the insertion point
			while((it != _chromosomes.end()) && 
				  (it->getFitness() > _nextGen[i].getFitness()) )
				it++;
			_chromosomes.insert(it, _nextGen[i]);
		}
	}
	_nextGen.clear();
	//we have a lot of excess members. clear them.
	while(_size != _chromosomes.size())
		_chromosomes.pop_back();

	return true;
}

bool ga2Population::_replaceSteadyStateNoDuplicates(void)
{
	int i;
	if(!_isSorted)
		return false;
	//note that we dont care about the replacement size
	for(i = 0; i < _nextGen.size(); ++i)
	{
		//we have to do an insertion sort. large elements first, small last
		if(_chromosomes.empty()) //if theres nothing there, just stuff it in there
			_chromosomes.push_back(_nextGen[i]);
		else //we have to look for the right place to put it...
		{
			//get an iterator
			std::vector<ga2Chromosome>::iterator it = _chromosomes.begin();
			//loop until we find a duplicate or the end. this is not
			//efficient, i realise...but other methods just seem to crash
			while( (it != _chromosomes.end()) && 
				   (it->getFitness() != _nextGen[i].getFitness()) )
				it++;
			if(it == _chromosomes.end()) //we've reached the end of the list, and
						 //can insert normally, as it is not a duplicate
			{
				it = _chromosomes.begin();
				while( (it != _chromosomes.end()) && 
					   (it->getFitness() > _nextGen[i].getFitness()) )
					it++;
				_chromosomes.insert(it, _nextGen[i]);
			}
		}
	}
	_nextGen.clear();
	//we have a lot of excess members. clear them.
	while(_size != _chromosomes.size())
		_chromosomes.pop_back();

	return true;
}

bool ga2Population::_replaceGenerational(void)
{
	int i;
	//if (!_isSorted)
	_chromosomes.clear(); //we dont want to clear this list if its all sorted.
	//note that we dont care about the replacement size
	for(i = 0; i < _nextGen.size(); ++i)
	{
		if(!_isSorted)
			_chromosomes.push_back(_nextGen[i]);
		else //we have to do an insertion sort. large elements first, small last
		{
			if(_chromosomes.empty()) //if theres nothing there, just stuff it in there
				_chromosomes.push_back(_nextGen[i]);
			else //we have to look for the right place to put it...
			{
				//get an iterator
				std::vector<ga2Chromosome>::iterator it = _chromosomes.begin();
				//loop until we find the insertion point
				while((it->getFitness() > _nextGen[i].getFitness()) &&
					  (it != _chromosomes.end()) )
					it++;
				_chromosomes.insert(it, _nextGen[i]);
			}

		}
	}
	_nextGen.clear();
	if(_isSorted) //if it is sorted, then we have a lot of excess members. clear them.
		while(_size < _chromosomes.size())
			_chromosomes.pop_back();

	return true;
}

/**
 * \param ranges A vector containing the upper bound for the values of each
 * gene.
 *
 * Sets the upper bounds for each gene in the chromosome.
 */
void ga2Population::setMaxRanges(std::vector<float> ranges)
{
	if(ranges.size() != _chromoSize)
		return;
	_chromoMaxRanges.clear();
	int i;
	for(i = 0; i < ranges.size(); ++i)
	{
		_chromoMaxRanges.push_back(ranges[i]);
	}
	for(i = 0; i < _chromosomes.size(); ++i)
		_chromosomes[i].setMaxRanges(ranges);
}

/**
 * \param ranges A vector containing the lower bound for the values of each
 * gene.
 *
 * Sets the lower bounds for each gene in the chromosome.
 */
void ga2Population::setMinRanges(std::vector<float> ranges)
{
	if(ranges.size() != _chromoSize)
		return;
	_chromoMinRanges.clear();
	int i;
	for(i = 0; i < ranges.size(); ++i)
	{
		_chromoMinRanges.push_back(ranges[i]);
	}
	for(i = 0; i < _chromosomes.size(); ++i)
		_chromosomes[i].setMinRanges(ranges);
}

/**
 * Returns a vector of genes representing the best fit chromosome in
 * the population.
 */
std::vector<ga2Gene> ga2Population::getBestFitChromosome(void)
{
	int i;
	double f = 0;
	for(i = 0; i < _size; ++i)
	{
		f = _chromosomes[i].getFitness();
		if(f == _maxFitness)
			return _chromosomes[i].getGenes();
	}
	return {};
}

/**
 * Serialises the population to a stream.
 */
std::ostream& operator<< ( std::ostream &o, ga2Population &pop )
{
	//first output size and ranges
	o << pop._size << " " << pop._chromoSize <<"\n";
	int i;
	for(i = 0; i < pop._chromoSize; ++i)
		o << pop._chromoMaxRanges[i] << " ";
	o << "\n";
	for(i = 0; i < pop._chromoSize; ++i)
		o << pop._chromoMinRanges[i] << " ";
	o << "\n";
	//now, output each chromosome
	for(i = 0; i < pop._size; ++i)
		o << pop._chromosomes[i];
	o << "\n";

	return o;
}

/**
 * Reads a serialised population from a stream.
 */
std::istream& operator>> ( std::istream &in, ga2Population &pop )
{
	//first, read in sizes and ranges;
	in >> pop._size >> pop._chromoSize;
	int i;
	pop._chromosomes.clear();
	pop._chromoMaxRanges.clear();
	pop._chromoMinRanges.clear();
	float f;
	for(i = 0; i < pop._chromoSize; ++i)
	{
		in >> f;
		pop._chromoMaxRanges.push_back(f);
	}
	for(i = 0; i < pop._chromoSize; ++i)
	{
		in >> f;
		pop._chromoMinRanges.push_back(f);
	}
	//now, grab the chromosomes themselves
	for(i = 0; i < pop._chromoSize; ++i)
	{
		ga2Chromosome c(pop._chromoSize);
		in >> c;
		c.setMaxRanges(pop._chromoMaxRanges);
		c.setMinRanges(pop._chromoMinRanges);
		c.setEvalFunc(pop._evalFunc);
		pop._chromosomes.push_back(c);
	}

	return in;
}
