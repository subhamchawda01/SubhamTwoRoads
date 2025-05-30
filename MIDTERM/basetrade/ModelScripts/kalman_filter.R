#!/usr/bin/env Rscript

args <- commandArgs(trailingOnly = TRUE);

if ( length ( args ) < 5 )
{
  stop ( " Usage: <ilist_file> <regdata_file>  <model_file> <max_size:pred_dur:update_dur:thres factor:num_samples_per_update:init_covar_factor:use_computed_weights> <feature_selection_algo> \n " ) ;
}
i=1;
ilist_file = args[i];
i = i+1;
reg_data_file = args[i];
i = i+1;
model_file = args[i];
i = i+1;
temp_str = args[i];
i = i+1;
feature_selection_algo = args[i];
i = i+1;

buildFilter_2 = function( param, indicator_df, init_state, init_covar = diag(init_state*init_state ) )
{
  require("dlm")
  param <- exp(param);
  return(  dlmModReg( X=data.frame(indicator_df), addInt = FALSE ,dV=param[1], dW=param[-1] ,m0 = init_state, C0 =  init_covar ) )   ;
  
}


estimate_kalman_params = function( reg_data, lm_weights , quick_and_dirty, num_splits = 20 )
{
  require("dlm")
  
   weights_df = data.frame(); 
   names = c();
 
  for( i in 1:ncol(reg_data) )
  {
   names[i] = paste("V",i, sep = "" );
  }

   names(reg_data) = names;

  
  for( i in 1:num_splits ) { 
    ind = seq(from = i, to = nrow( reg_data ), by = 30 ); 
    temp_data = reg_data[ind,]; 
    lm = lm(V1~.+0, temp_data );
    weights_df = rbind(weights_df, lm$coefficients ) ;  
  }
 
  covar = cov ( weights_df );
  
  if( length(lm_weights ) == 1){ covar = diag(1)*cov(weights_df) }
  
    init_guess = c();
 
  for( i in 1:ncol(reg_data) )
  {    
    if (i == 1 ) {
      sum_vars = as.matrix(reg_data[,-1]) %*% lm_weights;
      res = reg_data[,1] - sum_vars ;
      var = log ( var ( res ) ) ;       
 
    }else {
    var = log( covar[i-1,i-1] ) ;   
    }

   if ( is.na(var)  || is.nan(var) ) {
      var = 1e-6;
   } 
 
    init_guess = c( init_guess, var );
  }

 
  #covar = diag(c(lm_weights*lm_weights));
  #if( length(lm_weights ) == 1){ covar = diag(1)* lm_weights*lm_weights}
  
  init_state = lm_weights;
  init_covar = c();

  for( i in 1:ncol(covar ) ) {
   init_covar = c( init_covar, covar[i,i] ) ;
  }

  indicator_df = data.frame(reg_data[,-1]);
  
  buildFilter = function(param)
  {
    require("dlm")
    param <- exp(param);
    indicator_df = reg_data[,-1];
    return( dlmModReg( X=indicator_df, addInt = FALSE ,dV=param[1], dW= param[-1]  ,m0 = init_state, C0 = init_covar ) ) ;
    
  }
  
  price_diff = reg_data[,1];
  
  if( !quick_and_dirty  ){
  mle = dlmMLE(y=price_diff, parm=init_guess, build=buildFilter, hessian=T,debug = FALSE );
  
  optimal_param = mle$par;
  
  optimal_dlm = buildFilter(optimal_param );
  
  return(list( "param" = optimal_param, "obj" = optimal_dlm, "lm_weights" = lm_weights, "covar" = init_covar ) );
 }else{
     return( list( "param" = init_guess,  "lm_weights" = lm_weights, "covar" = init_covar ) );
   }
}



kalman_seqrep = function( ilist_file, reg_data_file, max_size, kalman_model_file, pred_dur, update_dur,thres_factor, num_samples_per_update, init_covar_factor, quick_and_dirty ,use_computed_weights  )
{
  require("leaps");
  data <- read.table(reg_data_file);
  indicators_ind = c();
  coef = c();

  x = data[,2:ncol(data)];
  y = data[,1];
  reg_data = data;
  reg_col = names(data);

if( use_computed_weights == 1) {
  if( max_size >= ncol(x) )
  {
    reg_data = data;
  }
  if( max_size < ncol(x) )
  {
    fea_obj =  regsubsets( x=as.matrix(x), y = y, nbest = 1, nvmax = max_size, method = "seqrep" ) ;
    coef = coef(fea_obj, max_size);
    coef = coef[!is.na(coef) ];
    reg_col =  c("V1", names(coef)[-1] );
    reg_data = data[reg_col];
  }

  lm = lm(formula = V1~.+0, data = reg_data );
  coef = coef(lm) ;
  names = names(coef);

  indicators_ind = c();
  for( i in 1:length(names))
  {
    indicators_ind[i] =  as.numeric ( strsplit( x = names[i], split = 'V',  ) [[1]] [2] )-1  ;
  }
}else{
 con  = file(ilist_file);
   temp_lines = readLines(con = con );
   close(con);
   curr_ind = 1;

   for( i in 1:length(temp_lines) ){
    l1  = temp_lines[i];
    l1_tokens = strsplit(l1, split = " ")[[1]];
   if( l1_tokens[1] == "INDICATOR" ){
    indicators_ind = c(indicators_ind, curr_ind);
    coef = c( coef, as.numeric(l1_tokens[2]) );
    curr_ind = curr_ind +1;
   } 

   }

}
  kalman_data = reg_data;
  mle = estimate_kalman_params(kalman_data,coef,quick_and_dirty);
  param = exp(mle$param);
  obs_var = param[1];
  param = param[-1]*init_covar_factor;

 

  init_state =  mle$lm_weights ;
 #init_covar =  init_state*init_state*init_covar_factor ;
  init_covar = mle$covar;

  system( paste( ">", kalman_model_file, sep="" ) ,intern=TRUE);
  con  = file(ilist_file);
  lines = readLines(con = con );

  close(con);

  first_line = lines[1];
  tokens = strsplit(first_line,split = " ")[[1]];
  product = tokens[3];
  price_type = tokens[4];

  price_indicator = paste("INDICATOR ", pred_dur, ":", update_dur, ":", obs_var,":",thres_factor,":",num_samples_per_update, ":0"," " ,"SimplePriceType", " ", product," ",price_type , sep = ""  );

  second_line = "MODELMATH ONLINELINEAR CHANGE";

  write(first_line,file= kalman_model_file,append=TRUE);
  write(second_line,file= kalman_model_file,append=TRUE);

  offset = 3;
  if( substr(lines[3], start =1, stop = 14 ) != "INDICATORSTART" )
  {
   offset = offset+1;
   write(lines[3], file= kalman_model_file,append=TRUE );
  }

  write("INDICATORSTART",file= kalman_model_file,append=TRUE);
  write(price_indicator,file= kalman_model_file,append=TRUE);


  current_index = 1;
  for ( i in indicators_ind  )
  {
    i = i + offset;
    line = lines[i];

    if(line =="" )
    { next; }
    if(substr(x, start = 1, stop = 1 ) == "#" )
    { next; }

    tokens = strsplit(line, split = " ")[[1]];

     str = paste(init_state[current_index],init_covar[current_index],param[current_index],current_index,sep=":"); 


    tokens[2] = str;
    indicator_str = "INDICATOR";
    for ( j in 2:length(tokens) )
    {
      indicator_str = paste(indicator_str, tokens[j], sep =  " ");
    }
    if ( !( is.na( init_state[current_index] ) || is.nan ( init_state[current_index] ) ) ) {
       write( indicator_str,file= kalman_model_file,append=TRUE );
    } 

    current_index = current_index+1;
  }

  write("INDICATOREND",file= kalman_model_file,append=TRUE );


}





kalman_slasso = function( ilist_file, reg_data_file, max_size, kalman_model_file, pred_dur, update_dur,thres_factor, num_samples_per_update, init_covar_factor, quick_and_dirty, use_computed_weights   )
{
  require("leaps");
  data <- read.table(reg_data_file);
  indicators_ind = c();
  weights = c();

if( use_computed_weights == 1){  
  weights_file = tempfile();
  call_str = paste( "~/basetrade_install/ModelScripts/call_slasso.pl", " ", reg_data_file, " ", max_size, " ",weights_file );
  system( call_str, intern = TRUE);
  
  
  temp_df = read.table(weights_file);
  indicators_ind = temp_df[,1];
  weights = temp_df[,2];
 }else{

   con  = file(ilist_file);
   temp_lines = readLines(con = con );
   close(con);
   curr_ind = 1;

   for( i in 1:length(temp_lines) ){
    l1  = temp_lines[i];
    l1_tokens = strsplit(l1, split = " ")[[1]];
   if( l1_tokens[1] == "INDICATOR" ){
    indicators_ind = c(indicators_ind, curr_ind);
    weights = c( weights, as.numeric(l1_tokens[2]) );
    curr_ind = curr_ind +1;
   } 

   }

} 
  
  kalman_data = data[,c(1,indicators_ind+1)];
  mle = estimate_kalman_params(kalman_data, weights, quick_and_dirty);
  param = exp(mle$param);
  obs_var = param[1];
  param = param[-1]*init_covar_factor;
  init_state =  mle$lm_weights ;
  #init_covar =  init_state*init_state*init_covar_factor ;
   init_covar = mle$covar;
  
  file.create(kalman_model_file);
  #system( paste( ">", kalman_model_file, sep="" ) ,intern=TRUE);
  con  = file(ilist_file);
  lines = readLines(con = con );
  
  close(con);
  
  first_line = lines[1];
  tokens = strsplit(first_line,split = " ")[[1]];
  product = tokens[3];
  price_type = tokens[4];
  
  price_indicator = paste("INDICATOR ", pred_dur, ":", update_dur, ":", obs_var,":",thres_factor,":",num_samples_per_update, ":0"," ", "SimplePriceType", " ", product," ",price_type , sep = ""  );
  
  second_line = "MODELMATH ONLINELINEAR CHANGE";
  
  write(first_line,file= kalman_model_file,append=TRUE);
  write(second_line,file= kalman_model_file,append=TRUE);
  
  offset = 3;
  flag = TRUE;
  temp_i = 3;
  
  while(flag)
  {
    if( substr(lines[temp_i], start =1, stop = 14 ) != "INDICATORSTART" )
    {      
      write(lines[temp_i], file= kalman_model_file,append=TRUE );
      offset = offset + 1;
      temp_i = temp_i + 1;
    }else{
      
      flag = FALSE;
    }
  }
  
  
  write("INDICATORSTART",file= kalman_model_file,append=TRUE);
  write(price_indicator,file= kalman_model_file,append=TRUE);
  
  current_index = 1;
  for ( i in indicators_ind  )
  {
    i = i + offset;
    line = lines[i];
    
    if(line =="" )
    { next; }
    if(substr(line, start = 1, stop = 1 ) == "#" )
    { next; }
    
    tokens = strsplit(line, split = " ")[[1]];
    
     str = paste(init_state[current_index],init_covar[current_index],param[current_index],current_index,sep=":"); 


    tokens[2] = str;
    indicator_str = "INDICATOR";
    for ( j in 2:length(tokens) )
    {
      indicator_str = paste(indicator_str, tokens[j], sep =  " ");
    }
   if ( !( is.na( init_state[current_index] ) || is.nan ( init_state[current_index] )  ) ) { 
    write( indicator_str,file= kalman_model_file,append=TRUE );
   }
    
    current_index = current_index+1;
  }
  
  write("INDICATOREND",file= kalman_model_file,append=TRUE );
  
  
}

kalman_siglr = function( ilist_file, reg_data_file, max_size, kalman_model_file, pred_dur, update_dur,thres_factor, num_samples_per_update, init_covar_factor, quick_and_dirty, use_computed_weights, domination_penalty, penalty   )
{
  require("leaps");
  data <- read.table(reg_data_file);
  indicators_ind = c();
  weights_alpha = c();
  weights_beta = c();

if( use_computed_weights == 1 ){  
  temp_weights_file = tempfile();
  call_str = paste( "~/basetrade_install/ModelScripts/SIGLR_grad_descent_3.R", " ", reg_data_file, " ", temp_weights_file, " " ,max_size, " "  ,domination_penalty," ", penalty, sep = ""  );
  system( call_str, intern = TRUE);
 
   con  = file(temp_weights_file);
   temp_lines = readLines(con = con );
   close(con);

   weights_file = tempfile(); 
   
   for( i in 1:length(temp_lines) )
   {
    l1 = temp_lines[i];
    if( strsplit(l1, split = " ")[[1]][1] == "OutCoeff" ){ 
     write(l1,file=weights_file,append=TRUE);
    }
   }
 
  
  temp_df = read.table(weights_file);
  indicators_ind = temp_df[,2] +1;
  weights_alpha = temp_df[,3];
  weights_beta = temp_df[,4];
}else{
   con  = file(ilist_file);
   temp_lines = readLines(con = con );
   close(con);
   curr_ind = 1;

   for( i in 1:length(temp_lines) ){
    l1  = temp_lines[i];
    l1_tokens = strsplit(l1, split = " ")[[1]];
   if( l1_tokens[1] == "INDICATOR" ){
    indicators_ind = c(indicators_ind, curr_ind);
    
    temp_tokens_2 = strsplit(l1_tokens[2], split= ":" )[[1]];
    this_alpha = as.numeric(temp_tokens_2[1]);  
    this_beta = as.numeric(temp_tokens_2[2]); 

    weights_alpha = c( weights_alpha, this_alpha );
    weights_beta  = c( weights_beta, this_beta );     

    curr_ind = curr_ind +1;  
   } 

   }
} 
  X_train = data[, indicators_ind+1 ] ;
  T =  X_train * t(replicate(nrow(X_train),weights_alpha)) ;
  new_X_train = 1/(1 + exp(-T)) - 0.5;
  kalman_data =  cbind( data[,1], new_X_train );
 
  mle = estimate_kalman_params(kalman_data, weights_beta, quick_and_dirty);
  param = exp(mle$param);
  obs_var = param[1];
  param = param[-1];
  init_state =  mle$lm_weights ;
  #init_covar =  init_state*init_state*init_covar_factor ;
  init_covar = mle$covar
  
  file.create(kalman_model_file);
  system( paste( ">", kalman_model_file, sep="" ) ,intern=TRUE);
  con  = file(ilist_file);
  lines = readLines(con = con );
  
  close(con);
  
  first_line = lines[1];
  tokens = strsplit(first_line,split = " ")[[1]];
  product = tokens[3];
  price_type = tokens[4];
  
  price_indicator = paste("INDICATOR ", pred_dur, ":", update_dur, ":", obs_var,":",thres_factor,":",num_samples_per_update, ":0"," ", "SimplePriceType", " ", product," ",price_type , sep = ""  );
  
  second_line = "MODELMATH ONLINESIGLR CHANGE";
  
  write(first_line,file= kalman_model_file,append=TRUE);
  write(second_line,file= kalman_model_file,append=TRUE);
  
  offset = 3;
  flag = TRUE;
  temp_i = 3;
  
  while(flag)
  {
    if( substr(lines[temp_i], start =1, stop = 14 ) != "INDICATORSTART" )
    {      
      write(lines[temp_i], file= kalman_model_file,append=TRUE );
      offset = offset + 1;
      temp_i = temp_i + 1;
    }else{
      
      flag = FALSE;
    }
  }
  
  
  write("INDICATORSTART",file= kalman_model_file,append=TRUE);
  write(price_indicator,file= kalman_model_file,append=TRUE);
  
  current_index = 1;
  for ( i in indicators_ind  )
  {
    i = i + offset;
    line = lines[i];
    
    if(line =="" )
    { next; }
    if(substr(line, start = 1, stop = 1 ) == "#" )
    { next; }
    
    tokens = strsplit(line, split = " ")[[1]];
    
     str = paste(weights_alpha[current_index],init_state[current_index],init_covar[current_index],param[current_index],current_index,sep=":"); 

    tokens[2] = str;
    indicator_str = "INDICATOR";
    for ( j in 2:length(tokens) )
    {
      indicator_str = paste(indicator_str, tokens[j], sep =  " ");
    }
    if ( !(is.na( init_state[current_index] ) || is.nan ( init_state[current_index] ) ) ) {
    write( indicator_str,file= kalman_model_file,append=TRUE );
    }
    
    current_index = current_index+1;
  }
  
  write("INDICATOREND",file= kalman_model_file,append=TRUE );
  
  
}

if(feature_selection_algo == "KALMAN" )
{ 
temp_str = strsplit(temp_str, split = ":" )[[1]];
max_size = as.numeric(temp_str[1]);
pred_dur = as.numeric(temp_str[2] );
update_dur =  as.numeric(temp_str[3]);
thres_factor = as.numeric(temp_str[4]);
num_samples_per_update = as.numeric(temp_str[5]);
init_covar_factor = as.numeric(temp_str[6]);
use_computed_weights = 1;

if( length( temp_str ) > 6 )
{
use_computed_weights = as.numeric(temp_str[7]);
}

kalman_seqrep ( ilist_file, reg_data_file, max_size, model_file, pred_dur, update_dur,thres_factor, num_samples_per_update, init_covar_factor,quick_and_dirty = FALSE, use_computed_weights   ); 

}

if(feature_selection_algo == "KALMAN_2" )
{ 
temp_str = strsplit(temp_str, split = ":" )[[1]];
max_size = as.numeric(temp_str[1]);
pred_dur = as.numeric(temp_str[2] );
update_dur =  as.numeric(temp_str[3]);
thres_factor = as.numeric(temp_str[4]);
num_samples_per_update = as.numeric(temp_str[5]);
init_covar_factor = as.numeric(temp_str[6]);
use_computed_weights = 1;

if( length( temp_str ) > 6 )
{
use_computed_weights = as.numeric(temp_str[7]);
}


kalman_seqrep ( ilist_file, reg_data_file, max_size, model_file, pred_dur, update_dur,thres_factor, num_samples_per_update, init_covar_factor,quick_and_dirty = TRUE , use_computed_weights  ); 


}


if(feature_selection_algo == "KALMAN_SLASSO" )
{ 
temp_str = strsplit(temp_str, split = ":" )[[1]];
max_size = as.numeric(temp_str[1]);
pred_dur = as.numeric(temp_str[2] );
update_dur =  as.numeric(temp_str[3]);
thres_factor = as.numeric(temp_str[4]);
num_samples_per_update = as.numeric(temp_str[5]);
init_covar_factor = as.numeric(temp_str[6]);
use_computed_weights = 1;

if( length( temp_str ) > 6 )
{
use_computed_weights = as.numeric(temp_str[7]);
}


kalman_slasso ( ilist_file, reg_data_file, max_size, model_file, pred_dur, update_dur,thres_factor, num_samples_per_update, init_covar_factor,quick_and_dirty = FALSE , use_computed_weights  ); 
}

if(feature_selection_algo == "KALMAN_SLASSO_2" )
{ 
temp_str = strsplit(temp_str, split = ":" )[[1]];
max_size = as.numeric(temp_str[1]);
pred_dur = as.numeric(temp_str[2] );
update_dur =  as.numeric(temp_str[3]);
thres_factor = as.numeric(temp_str[4]);
num_samples_per_update = as.numeric(temp_str[5]);
init_covar_factor = as.numeric(temp_str[6]);
use_computed_weights = 1;

if( length( temp_str ) > 6 )
{
use_computed_weights = as.numeric(temp_str[7]);
}

kalman_slasso ( ilist_file, reg_data_file, max_size, model_file, pred_dur, update_dur,thres_factor, num_samples_per_update, init_covar_factor,quick_and_dirty = TRUE, use_computed_weights   ); 
}

if(feature_selection_algo == "KALMAN_SIGLR" )
{ 
temp_str = strsplit(temp_str, split = ":" )[[1]];
max_size = as.numeric(temp_str[1]);
pred_dur = as.numeric(temp_str[2] );
update_dur =  as.numeric(temp_str[3]);
thres_factor = as.numeric(temp_str[4]);
num_samples_per_update = as.numeric(temp_str[5]);
init_covar_factor = as.numeric(temp_str[6]);
use_computed_weights = as.numeric(temp_str[7]);
domination_penalty = temp_str[8];
penalty = temp_str[9];


kalman_siglr ( ilist_file, reg_data_file, max_size, model_file, pred_dur, update_dur,thres_factor, num_samples_per_update, init_covar_factor,quick_and_dirty = FALSE , use_computed_weights,domination_penalty, penalty  ); 

}

if(feature_selection_algo == "KALMAN_SIGLR_2" )
{ 

temp_str = strsplit(temp_str, split = ":" )[[1]];
max_size = as.numeric(temp_str[1]);
pred_dur = as.numeric(temp_str[2] );
update_dur =  as.numeric(temp_str[3]);
thres_factor = as.numeric(temp_str[4]);
num_samples_per_update = as.numeric(temp_str[5]);
init_covar_factor = as.numeric(temp_str[6]);
use_computed_weights = as.numeric(temp_str[7]);
domination_penalty = temp_str[8];
penalty = temp_str[9];


kalman_siglr ( ilist_file, reg_data_file, max_size, model_file, pred_dur, update_dur,thres_factor, num_samples_per_update, init_covar_factor,quick_and_dirty = TRUE, use_computed_weights, domination_penalty, penalty   ); 

}
