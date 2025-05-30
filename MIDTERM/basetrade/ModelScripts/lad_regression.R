#!/usr/bin/env Rscript

.libPaths("/apps/R/root/library/") ; # probably not needed after ~/.Renviron change
suppressPackageStartupMessages ( require (SparseM) ) ; 
suppressPackageStartupMessages ( require (getopt) ) ; 
suppressPackageStartupMessages ( library ( "quantreg" ) ) ;


#usage arg[1] = reg file, arg[2] = output file arg[3]=tstat_cutoff_individual arg[4]=acceptable_tstat_decrease_ration

spec <- matrix ( c ( 
  'verbose'      , 'v', 0, "logical",
  'help'         , 'h', 0, "logical",
  'sample'       , 'n', 0, "integer",
  'model_sz'     , 's', 2, "integer",
  'tstat_cutoff' , 'c', 2, "double",
  'tstat_ratio'  , 'r', 2, "double",
  'tau'          , 't', 2, "double",
  'algo'         , 'a', 2, "character", #heuristic or algo
  'regfile'      , 'i', 1, "character", #input
  'outfile'      , 'o', 1, "character" ), byrow = TRUE, ncol = 4 ) ;
opt = getopt ( spec ) ;
if ( is.null ( opt$verbose ) ) { opt$verbose = FALSE };
if ( is.null ( opt$help ) ) { opt$help = FALSE };
if ( is.null ( opt$sample ) ) { opt$sample = 100000 };
if ( is.null ( opt$model_sz ) ) { opt$model_sz = 12 };
if ( is.null ( opt$tstat_cutoff ) ) { opt$tstat_cutoff = 5 };
if ( is.null ( opt$tstat_ratio ) ) { opt$tstat_ratio = 0.5 };
if ( is.null ( opt$tau ) ) { opt$tau = 0.5 };
if ( is.null ( opt$algo ) ) { opt$algo = "L1_VAL_SORT"};
if ( is.null ( opt$regfile ) ) { opt$help = TRUE };
if ( is.null ( opt$outfile ) ) { opt$help = TRUE };

#TODO filter is flag is set
#TODO try sorting on tstat in 1st step as heuristic

if ( opt$help == TRUE ) {  
  cat ( "optional flags\n" );
  cat ( "  --verbose      or -v   : Prints excess logging\n");
  cat ( "  --help         or -h   : displays this message\n");
  cat ( "optional params\n" );
  cat ( "  --sample     or -n     : sample n rows randomly from input regfile, default 100000\n");
  cat ( "  --model_sz     or -s   : max predictors in model, default 12\n");
  cat ( "  --tstat_cutoff or -c   : min abs individual tstat for any chosen predictor, default 5\n");
  cat ( "  --tstat_ratio  or -r   : least acceptable value of tstat in incremnetal model / tstat in original model, default 0.5\n");
  cat ( "  --tau          or -t   : tau value in quantile regression, default 0.5\n");
  cat ( "  --algo         or -a   : heuristic or algo used in first step. no variants currently\n");
  cat ( "necessary params\n" );
  cat ( "  --regfile      or -i   : input regdata file\n");
  cat ( "  --outfile      or -o   : output file written by this program containing model coefficient and summary\n");
  q();
}

#read.table is too slow for large matrix ( size >= 1G ), hence the method below
nRowCmd <- paste ( "wc -l ", opt$regfile, " | awk '{print $1}' " );
nColCmd <- paste ( "head -n 1  ", opt$regfile, " | wc -w  | awk '{print $1}' " ) ;
numRows <- as.numeric ( try ( system(nRowCmd, intern=TRUE) ) );
numCols <- as.numeric ( try ( system (nColCmd, intern=TRUE) ) );
data <- matrix ( scan ( opt$regfile, n = numRows * numCols), numRows, numCols, byrow = TRUE ) ;

if ( opt$verbose ) 
  print ( sprintf ( "read data numRows %d, numCols %d", numRows, numCols ) );
  
#sample data
if ( opt$sample > 0 && opt$sample < numRows ) {
  data <- data[sort(sample(seq(1,numRows), opt$sample)), ];
}

y <- data[,1];
data <- data[,-1];
data <- scale ( data ) ;

numCols <- ncol ( data  ) ; 

l1_reg <- NULL;

stdev_ <- attr ( data, "scaled:scale" );
mean_ <- attr ( data, "scaled:center" );
sharpe_ <- abs ( mean_ ) / stdev_ ;

for (i in seq ( 1, numCols ) ) {
  if ( stdev_[i] > 0 && sharpe_ [i] < .22 ) {
    tmp <- summary ( rq ( y ~ data [, i ], tau=opt$tau, method = "fn" ), se = "nid" ) ;
    l1_reg <- rbind (l1_reg, c(i, tmp$coefficients[2,]) );
    if ( opt$verbose ) 
      print ( sprintf ( "Computed regression for %d", i ) ) ;
  }
}

#iteratively build now
#2 <- sorted on l1 reg coefficient
sorted_index <- rev ( order (abs ( l1_reg[,2] ) ) );

model_index <- c ();
fin_model<-NULL;
tmp_new_model<-NULL;



for ( i in 1:length(sorted_index) ) {
  
  if ( length ( model_index ) == opt$model_sz ) {
    print ( sprintf ("max model size of %d attained", opt$model_sz) );
    break;
  }
          
  if ( abs ( l1_reg [sorted_index[i], 4] ) > opt$tstat_cutoff ) {
    if ( opt$verbose ) 
      print ( sprintf ( "check for candidate %d (tstat: %f )",  l1_reg [sorted_index[i], 1], l1_reg [sorted_index[i], 4] ) );
    if ( length ( model_index ) == 0 ) {
      tmp_new_model <- tryCatch( summary ( rq ( y ~ data [, l1_reg [sorted_index[i],1] ], tau=opt$tau, method = "fn" ), se="nid" ), error=function(e) e ) ;
      if ( is.null ( tmp_new_model$coefficients ) == FALSE ) {
        model_index <- c( l1_reg [sorted_index[i],1] ) ;
        if ( opt$verbose ) {
          print ( sprintf ( "added index %d",  l1_reg [sorted_index[i]][1] ) );
          print ( model_index ) ;
        }
        fin_model <- tmp_new_model;
      }
    }
    else {
      tmp_model_indx <- c(model_index, l1_reg [sorted_index[i]][1]);      
      tmp_new_model <- tryCatch( summary( rq(y~data[,tmp_model_indx], tau=opt$tau, method="fn"), se="nid" ), error=function(e) e) ;
      if ( is.null ( tmp_new_model$coefficients ) == FALSE ) {
        #check if tstat constraints are not violated
        
        rejected <- FALSE;
        tstat_ratio <- 0;

        #validate if previous tstats are stable in view of adding a new predictor
        for ( indx in seq(2, length(model_index) + 1) ) {
          ts_prev <- fin_model$coefficients[indx,][3];
          ts_curr <- tmp_new_model$coefficients[indx,][3];
          
          tstat_ratio = ts_curr / ts_prev;
          if ( abs ( ts_curr ) < opt$tstat_cutoff ){
            rejected <- TRUE;
            if ( opt$verbose )
              print ( sprintf ( "rejected due to tstat in propsed model less than threshold for index %d , tstat %f",  model_index[indx-1], ts_curr ) );
            break;
          }
          if (rejected == FALSE && tstat_ratio < opt$tstat_ratio ) { # we dont want t stat to decrease considerably
            rejected <- TRUE;
            if ( opt$verbose )
              print ( sprintf ( "rejected due to tstat decrease from previous model is considerable. index %d, tstat_now %f, tstat_previous_model %f, tstat_ratio %f",  model_index[indx-1], ts_curr, ts_prev, tstat_ratio ) );
            break;
          }
        }
        
        #check is tstat of the current predictor is fine
        if ( rejected == FALSE ) {
          ts_single_model <- l1_reg [sorted_index[i],4];
          ts_temp_model <- tmp_new_model$coefficients[length ( model_index ) + 2,][3]
          tstat_ratio = ts_temp_model / ts_single_model;
          if ( abs ( ts_temp_model ) < opt$tstat_cutoff ){
            rejected <- TRUE;
            if ( opt$verbose )
              print ( sprintf ( "rejected due to tstat in propsed model less than threshold for index %d , tstat %f",  l1_reg [sorted_index[i],1], ts_temp_model ) );
          }
          if ( rejected == FALSE && tstat_ratio < opt$tstat_ratio ) { # we dont want t stat to decrease considerably
            rejected <- TRUE;
            if ( opt$verbose )
              print ( sprintf ( "rejected due to tstat in model much worse than in individual model. index %d, tstat_now %f, tstat_individual %f, ratio %f", l1_reg [sorted_index[i],1], ts_temp_model, ts_single_model, tstat_ratio) );
          }
        }
        
        if ( rejected == FALSE ) { 
        fin_model <- tmp_new_model
          model_index <- tmp_model_indx;
          if ( opt$verbose ) {
            print ( sprintf ( "added index %d",  l1_reg [sorted_index[i]][1] ) );
            print ( model_index ) ;
            print ( fin_model$coefficients ) ;
          }
        }        
      }
      else {
        if ( opt$verbose ) {
          print ( sprintf ( "rejected iteration %d due to consistency error\n",  l1_reg [sorted_index[i]][1] ) );
          print ( model_index ) ;
          print ( tmp_model_indx ) ;
        }
      }
    }
  }
}

if ( opt$verbose ) 
  print ( fin_model$coefficients ) ;

#compute nessary statistics to output
coeff <- fin_model$coefficients; coeff <- coeff [-1, ];
coeff <- cbind ( model_index -1 , coeff ) ;

pred_data <- data[, model_index] %*% coeff[,2];

r2_original <- sum ( y ^ 2 ) ;
r2_residual <- sum ( (y-pred_data) ^ 2 ) ; 
r_squared <- ( r2_original - r2_residual ) / r2_original;

cor_model <- cor ( y, pred_data );
y<-as.matrix(y);
sd_dep <- apply(y,2,sd) ;
sd_residual <- apply(y-pred_data,2,sd);
sd_model <- apply(pred_data,2,sd) ;
mse <- mean ( (y-pred_data)^2 );
abs_err <- mean ( abs ( y-pred_data ) ) ;

#adjust model coefficients for data scaling done initially
coeff[,2] <- coeff [,2] / attr (data,"scaled:scale") [model_index] ;

#write
sink ( opt$outfile ) ;
cat ("#OutCoeff indep-index value error t-stat p\n");
for ( i in 1:nrow(coeff) ) { cat("OutCoeff "); cat (coeff[i,]); cat ("\n"); }
cat ("RSquared "); cat (r_squared); cat ("\n");
cat ("Correlation "); cat (cor_model); cat ("\n");
cat ("StdevDependent "); cat (sd_dep); cat ("\n");
cat ("StdevResidual "); cat (sd_residual); cat ("\n");
cat ("StdevModel "); cat (sd_model); cat ("\n");
cat ("MSE "); cat (mse); cat ("\n");
cat ("MeanAbsError "); cat (abs_err); cat ("\n");
sink();
