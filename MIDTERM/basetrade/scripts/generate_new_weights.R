#!/usr/bin/env Rscript
.libPaths("/apps/R/root/library/")
suppressPackageStartupMessages( require('gtools') );
#################

# function to get n p-dimensional samples in the range { 0, 1, 2, k } such that the sum of n samples is s
GetStartingPopulation <- function ( p,n,k,s )
{
  permutations <- c();
  for ( i in seq(1,n) )
  {
    valid_permutation_found <- FALSE;
    while ( !valid_permutation_found )
    {
      # get a sample from the dirichlet distribution, the value 3 reduces the stdev in resulting samples. Works well for our use case
      # rescaling the samples to value s and rounding off get integer values.
      permutation <- ( s * rdirichlet ( 1,rep(3,p) ) );

      # if the generated sample satisfies the 
      if ( length( which ( permutation > k ) ) <= 0 )
      {
        valid_permutation_found <- TRUE;          
        permutation <- AdjustPermutationWeights ( permutation, s );
        permutations <- cbind ( permutations, permutation);                       

        # making a string for all the permutation so that we know whether the result for a permutation has been computed or not
        permutation_string <- paste(permutation, collapse='');
        already_computed <- c(already_computed, permutation_string );
      }
      else {
        next;    
      }
    }
  }
}

AdjustPermutationWeights <- function ( permutation, s )
{
  sdiff <- s - sum(permutation);
  if ( sdiff == 0 )
  {
    return permutation;
  }
  else if ( sdiff > 0 )
  {  
    offset <- 0;
    new_permutation <- permutation;
    for ( i in seq( 1, abs(sdiff) ) )
    {
      while ( new_permutation [ i + offset ] >= 5 )
      {
        offset <- offset + 1;        
      }
      new_permutation [ i + offset ] <- permutation [ i + offset ] + 1;
    }
  }
  else if ( sdiff < 0 )
  {    
    offset <- 0;  
    new_permutation <- permutation;
    for ( i in seq ( 1, abs(sdiff) ) )
    {      
      while ( new_permutation [ i + offset ] <= 0 )
      {
        offset <- offset + 1;
      }
      new_permutation [ i + offset ] <- permutation [ i  + offset ] - 1;
    }
  }
  return new_permutation;
}

CheckIfAlreadyComputed <- function ( permutation_string )
{
  if ( permutation_string %in% already_computed )
  {
    return TRUE;
  }    
  else
  {
    return FALSE;
  }
}

# SwitchMutation randomly selects two points indices and switches their weights. If the indices have same weights then, it chooses different indices.
# Also if a mutation is already there in our already computed set, then we come up with another mutation
SwitchMutation <- function ( permutation )
{
  valid_mutation_found <- FALSE;
  iterations <- 0;
  while ( valid_mutation_found || iterations > 10 )
  {
    iterations <- iterations + 1;    
    indices_to_swap <- sample ( 1:length(permutation), 2 );
    if ( permutation[indices_to_swap[1] == permutation[indices_to_swap[2] )
    {
      next;
    }
    else
    {
      new_permutation <- permutation;
      new_permutation[indices_to_swap[1]] <- permutation[indices_to_swap[2]];
      new_permutation[indices_to_swap[2]] <- permutation[indices_to_swap[1]];
      new_permutation_string <- paste ( new_permutation, collapse = '' );
      if ( CheckIfAlreadyComputed ( new_permutation_string ) )
      {
        next;
      }
      else 
      {
        valid_mutation_found <- TRUE;
        return new_permutation;
      }
    }
  }
  return NULL;
}

IsValidPermutation <- function ( permutation )
{
  if ( length(which(permutation > MAX_CONTRIBUTION ) ) >=  0 )
  {
    return FALSE;
  }
  else if ( length(which(permutation < 0 ) ) >= 0 )
  {
    return FALSE;
  }
  else if ( sum(permutation) != SUM_CONTRIBUTION )
  {
    return FALSE;
  }
  else
  {
    return TRUE;
  }
}


# Mutation selects two indices randomly and increases the contribution of one of them and reduces the contribution of the second.
Mutation <- function ( permutation )
{
  valid_mutation_found <- FALSE;
  iterations <- 0;
  while ( valid_mutation_found || iterations > 10 )
  {
    new_permutation <- permutation;
    iterations <- iterations + 1;
    indices_to_mutate <- sample (1:length(permutation),2 );
    new_permutation[indices_to_swap[1]] <- permutation[indices_to_swap[1]] + 1;
    new_permutation[indices_to_swap[2]] <- permutation[indices_to_swap[2]] - 1;    
    if ( !IsValidPermutation ( new_permutation ) )
    {
      next;
    }
    new_permutation_string <- paste(new_permutation, collapse = '' )
    if ( CheckIfAlreadyComputed ( new_permutation_string ) )
    {
      next;
    }
    valid_mutation_found <- TRUE; 
    return new_permutation;
  }
  return NULL;
}

# Average operator takes two permutations and takes their average and computes the average of the two permutations
Average <- function ( permutation1, permutation2 )
{
  valid_average_found <- FALSE;
  iterations <- 0;
  while ( valid_average_found || iterations > 10 )
  {
    iterations <- iterations + 1;
    new_permutation <- round ( 0.5 * permutation1 + 0.5 * permutation2 );    
    new_permutation <- AdjustPermutationWeights ( new_permutation, s );
    new_permutation_string <- paste ( new_permutation_string );
    if ( CheckIfAlreadyComputed ( new_permutation_string ) )
    {
      next;
    }
    else
    {
      valid_average_found <- TRUE;
      return new_permutation;
    }
  }
  return NULL;
}


# Shift operator takes in a permutation and shifts the permutation around that point
Shift <- function ( permutation )
{
  valid_shift_found <- FALSE:
  iterations <- 0;
  while ( valid_shift_found || iterations > 10 )
  {
    iterations <- iterations + 1;
    l <- length(permutation);
    indx <- sample ( 2:l, 1 );
    
    new_permutation <- permutation ;
    new_permutation[1:(l-indx+1)] <- permutation[indx:];
    new_permutation[(l-indx+2):] <- permutation[1:(indx-1)];
    new_permutation_string <- paste ( new_permutation_string );
    if ( CheckIfAlreadyComputed ( new_permutation_string ) )
    {
      next;
    }
    else
    {
      valid_shift_found <- TRUE;
      return new_permutation;
    }
  }
  return NULL;  
}

Dissimilarity <- function ( permutation1, permutation2 )
{
  return sum ( abs ( permutation1 - permutation2 ) ) ;
}


# select the better performing individuals in the population to be used for reproduction for the next iteration
Selection <- function ( population, scores, selected_population_size )
{  
  population_size <- length(population);
  selected_population_size <- ceil ( 0.25 * population_size );
  prob_weights <- ( seq ( 1, population_size ) ^ 2 ) / sum ( seq ( 1, population_size ) ^ 2 ) ;
  score_sorted_indices <- sort ( scores, index.return=TRUE )$ix;
  selected_indices <-  sample( score_sorted_indices, selected_population_size, prob = prob_weights ); 
  selected_population <- population[selected_indices,];
  return selected_population;
}

GetNewPopulation <- function ( population, scores )
{
  population_size <- length(population);
  selected_population_size <- ceil ( 0.25 * population_size );
  selected_population <- Selection ( population, scores, selected_population_size );
  shift_size <- selected_population_size;
  switch_mutation_size <- selected_population_size;  
  mutation_size <- selected_populattion_size;
  average_size <- population_size - 3 * selected_population_size;
  
  # Getting new offsprings through shift
  size <- 0;
  while ( size < shift_size )
  { 
    for ( i in seq(1,selected_population_size) )
    {
      new_permutation <- Shift ( selected_population[i] );
      if ( new_permutation != NULL )
      {  
        size <- size + 1;
        new_permutation_string <- paste ( new_permutation, collapse = '' );
        already_computed <- c(already_computed, new_permutation_string );
      }
    }
  }

  # Getting new offsprings through switchmutation
  size <- 0 ;
  while ( size < switch_mutation_size )
  {
    for ( i in seq(1,selected_population_size) )
    {
      new_permutation <- SwitchMutation ( selected_population[i] );
      if ( new_permutation != NULL )
      {
        size <- size + 1;
        new_permutation_string <- paste ( new_permutation, collapse = '' );
        already_computed <- c(already_computed, new_permutation_string );
      }
    }
  }
  
  # Getting new offsprings through mutation
  size <- 0;
  while ( size < mutation_size ) 
  {
    for ( i in seq (1,selected_population_size ) )
    {
      new_permutation <- Mutation ( selected_population[i] );
      if ( new_permutation != NULL )
      {
        size <- size + 1;
        new_permutation_string <- paste ( new_permutation, collapse = '' );
        already_computed <- c(already_computed, new_permutation_string );
      }
    }
  } 
  
  # Getting new offspring through average 
  size <- 0;
  while ( size <- average_size )
  {
    indices <- sample ( 1:selected_population_size, 2 ); 
    new_permutation <- Average ( selected_population[indices[1],], selected_population[indices[2],] );
    if ( new_permutation != NULL )
    {
      new_permutation_string <- paste ( new_permutation, collapse = '' );
      already_computed <- c(already_computed, new_permutation_string );
      size <- size + 1;
    }    
  }
}

WriteModelFile <- function ( header, footer, permutation, indicator_string, filename )
{
}

# Normalize the permutation  
NormalizePermutation <- function ( permutation, cov_mat, target_stdev )
{
  permutation <- permutation *  ( 1 / diag(cov_mat) );
  permutation_variance <- sum ( outer ( permutation, permutation ) * cov_mat );  
  permutation_stdev <- sqrt ( permutation_variance );
  normalization_factor <- target_stdev / permutation_stdev;
  new_permutation <- permutation * normalization_factor;
  return new_permutation;
}
