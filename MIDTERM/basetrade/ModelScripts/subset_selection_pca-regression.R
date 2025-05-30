#!/usr/bin/env Rscript
args <- commandArgs ( trailingOnly = TRUE );
if ( length(args) < 3 )
{
   stop ("USAGE : <script> <regdata> <max_model_size> <out_file>\n");
}
max_ind_cut_ <- as.numeric ( args[2] );
out_file_name_ <- args[3];

# load the input regress-able data file
yxdata_ <- as.matrix ( read.table ( args[1] ) );
scaled_yxdata_ <- scale ( yxdata_ ); # remove means and scale by stdev
used_scales_ <- attr ( scaled_yxdata_, "scaled:scale"); # store values used for scaling
remove ( yxdata_ ); # remove this from memory

# break into DEP, INDEP for future calls
tgt_ <- scaled_yxdata_ [,1]; 
indicators_ <- scaled_yxdata_ [ , 2:ncol ( scaled_yxdata_ )];

remove ( scaled_yxdata_ ); # remove this from memory

# cleaning. this should not be required normally
tgt_ [ is.nan ( tgt_)] <- 0;
indicators_ [ is.nan ( indicators_ )] <- 0;

# run pca on normalized indicators
pca_x_ <- princomp ( indicators_ );

# extract the eigen values of the indicators, 
# or the major ways ( sorted by variance explained ) 
# in which the indicators change together
eig_x_ <- with ( pca_x_, unclass ( loadings ) );

# compute hwo the eigen vectors are moving
# for instance if say the first eien vector 
# is some sort of aggreghation of book indicators
# trans_data_[,1] would be an average stdev-adjusted 
# value of those indicators
trans_data_ <- indicators_ %*% eig_x_;
remove ( indicators_ ); # remove from memory

# now regress the actual dependant on the 
# transformed data
out_pca_y_ <- lm ( tgt_ ~ 0 + trans_data_ );
remove ( tgt_ ); # remove from memory

coeff_pca_y_ <- as.matrix ( out_pca_y_$coefficients );
coeff_pca_y_ [ is.na ( coeff_pca_y_ ) ] <- 0; # to take into account coefficients that do not change

# to bring back coefficients to original space of DEP, INDEP
# project them back by the eigen matrix
coeff_sd_y_ <- coeff_pca_y_ [ , 1 ] %*% t(eig_x_);

# sort in decreasing order the absolute value of the coefficients
# and cut it off at whatever is the max number of indicators you want to have
# in your model. Store this value
topcut_value_ <- sort ( abs ( coeff_sd_y_ ), decreasing=TRUE ) [ max_ind_cut_ ];

# for every index that has mod-value less than this cutoff set it to 0
coeff_sd_y_ [ abs ( coeff_sd_y_ ) < topcut_value_ ] <- 0;

# since the data was descaled, bring it back to weights such that 
# they can be applied on the original model
# used_scales_[1] is the stdev of the dependant column. Hence it should be mutiplied
# the coefficients[j] should then be normalized by dividing by the stdev of the 
# jth indicator, which is at index (j+1) in used_scales_
final_coeff_ <- used_scales_[1] * ( coeff_sd_y_ / used_scales_ [ 2:(ncol(coeff_sd_y_) + 1) ] ) ;

# santize
final_coeff_ [ is.nan ( final_coeff_ ) ] <- 0 ;
# write to file
write.table ( t(final_coeff_), out_file_name_, row.names=F, col.names=F ) ;
