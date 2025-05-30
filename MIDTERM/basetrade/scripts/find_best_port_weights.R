#!/usr/bin/env Rscript
.libPaths("/apps/R/root/library/")

if ( exists( "verbose" ) ) {
    library('numDeriv');
      library('nloptr');
} else {
    msg.trap <- capture.output ( suppressPackageStartupMessages ( require('numDeriv') ) ) ;
      msg.trap <- capture.output ( suppressPackageStartupMessages ( require('nloptr') ) ) ;
}

#################
#Idea : To find a set of weights for a portfolio of sources that maximizes the correlation with dependent. Trivially the 
# portfolio should not have the dependent among the components
# Objective function: maximize the corr
MaxCorr <- function ( B ) {
  WCorr <- cor ( Y, X %*% B ) ;
  regularization_penalty <- 0;

  if ( regularization_method == "L2" )
  {
    regularization_penalty <- L2Norm ( B );
  }
  if ( regularization_penalty == "L1" )
  {
    regularization_penalty <- L1Norm ( B );
  }

  retVal <- -abs(WCorr) + regularization_penalty;
}

#Calculates the gradient using inbuilt R function
GradNumerical <- function ( B ) {
  retVal <- grad ( MaxCorr, B, method="simple" );
}

L2Norm <- function ( B ) {
  retVal <- lambda_weight * sum ( B * B );
}

L1Norm <- function ( B ) {
  retVal <- lambda_weight * sum ( abs ( B ) );
}

EqualityConstraint <- function ( B ) {     
  allones <- rep(1,num_port_components);
  B_square <- B * B;
  retVal <- allones %*% B_square - 1;
}

EqualityConstraintGrad <- function ( B) {
#retVal <- rep(1,num_port_components);
  retVal <- 2 * B;
}

GetListofDates <- function ( )
{
        cat(sprintf("~/basetrade/scripts/get_list_of_dates_for_shortcode.pl %s %s %s", shortcode, start_date, num_days_lookback),"\n" );
        dates <- system ( sprintf("~/basetrade/scripts/get_list_of_dates_for_shortcode.pl %s %s %s", shortcode, start_date, num_days_lookback),intern=TRUE );
        list_of_dates <<- unlist(strsplit(dates, "\\ "));
        print (list_of_dates);
}

GetPortfolioComponents <- function ( ) {
#	components <- system ( sprintf("grep \" %s \" %s", portfolio, portfolio_inputs_filename, num_days_lookback),intern=TRUE );
#	list_of_port_components <<-  unlist(strsplit(components, "\\ "));
#	list_of_port_components <<-  list_of_port_components[3:length(list_of_port_components)];
  shortcode_component <- system ( sprintf("grep SHC %s", portfolio_inputs_filename),intern=TRUE );
  list_of_port_components <<- unlist(strsplit(shortcode_component, "\\ "));
  list_of_port_components <<- list_of_port_components[!list_of_port_components == ""]
  list_of_port_components <<-  list_of_port_components[2:length(list_of_port_components)];

  num_port_components <<- length(list_of_port_components);

  sign_component <- system ( sprintf("grep SIGN %s", portfolio_inputs_filename),intern=TRUE );
  sign_vec <<- unlist(strsplit(sign_component, "\\ "));
  sign_vec <<- sign_vec[!sign_vec == ""]
  sign_vec <<- sign_vec[2:length(sign_vec)];
  sign_vec <<- as.numeric(sign_vec);

  if ( length(sign_vec) != length(list_of_port_components) ) {
    stop ("Length of Sign vec not equal to Length of portfolio vec\n");
  }

  if ( shortcode %in% list_of_port_components ) {
    stop ("Shortcode present in Portfolio. Try considering portfolio without the dep shortcode\n");
  }
  UBVec <<- rep(1, num_port_components );
  LBVec <<- rep(0, num_port_components );

#adjusting LBVec and UBVec for sign
  UBVec <<- (1/2) * ( UBVec + sign_vec );
  LBVec <<- sign_vec - UBVec ;
}

MakeIlist <- function ( ) {
	cat ("MODELINIT DEPBASE ",shortcode, " MktSizeWPrice MktSizeWPrice\nMODELMATH LINEAR CHANGE\nINDICATORSTART",file=ilist_filename);	    
	cat ( "\nINDICATOR 1.00 SimpleTrend ",shortcode," ", trend_secs, " MktSizeWPrice", file=ilist_filename, sep="", append=TRUE ); 
	for ( i in c(1:length(list_of_port_components) ) )
	{
		cat ( "\nINDICATOR 1.00 SimpleTrend ",list_of_port_components[i], " ", trend_secs, " MktSizeWPrice", file=ilist_filename, sep="", append=TRUE );		
	}
	cat ("\nINDICATOREND", file=ilist_filename, append=TRUE );
}

ComputeRegData <- function ( ) {
  t_dgen_outfile <- paste( work_dir, "t_dgen_outfile_", shortcode, "_", start_time, "_", end_time,sep="" );
  dgen_msecs <- 1000;
	dgen_l1events <- 15;
	dgen_trades <- 0;
	pred_algo <- "na_e3";
	pred_counter <- 32;
  to_print_on_eco <- "1";	
  for ( trading_date in list_of_dates )
  {
    datagen_exec <- paste(LIVE_BIN_DIR, "datagen", sep="");
#            timed_data_to_reg_data_exec <- paste(LIVE_BIN_DIR, "timed_data_to_reg_data", sep="");
    cat ( datagen_exec, " ", ilist_filename, " ", trading_date, " ", start_time, " ", end_time, " 22222 ", t_dgen_outfile, " ", dgen_msecs, " ", dgen_l1events, " ", dgen_trades, " 1\n");
    system ( sprintf ( "%s %s %s %s %s %s %s %s %s %s %s 2>/dev/null", datagen_exec, ilist_filename,  trading_date, start_time, end_time, 22222, t_dgen_outfile, dgen_msecs, dgen_l1events, dgen_trades, to_print_on_eco ) );

#           cat ( timed_data_to_reg_data_exec, " ", ilist_filename, " ", t_dgen_outfile, " ", pred_counter, " ", pred_algo, " ", t_regdata_outfile, "\n");
#            system ( sprintf ( "%s %s %s %s %s %s 2>/dev/null", timed_data_to_reg_data_exec, ilist_filename, t_dgen_outfile, pred_counter, pred_algo, t_regdata_outfile ) );

    system ( sprintf ( "cat %s >> %s", t_dgen_outfile, catted_data_filename ) );
    system( sprintf ( "rm -f %s", t_dgen_outfile ) );
  }	
}


list_of_port_components <- c();
sign_vec <- c();
LBVec <- c();
UBVec <- c();
num_port_components <- 0;

regdata <- c();
list_of_dates <- c();

USER <- Sys.getenv("USER");

work_dir <- paste( "/spare/local/", USER, "/BestPortData/", sep="" );
system ( sprintf ("mkdir -p %s", work_dir) );

LIVE_BIN_DIR <- paste("/home/",USER,"/LiveExec/bin/",sep="");

args = commandArgs( trailingOnly=TRUE )
if ( length(args) < 6 ) {
        stop ("USAGE : <script> <shortcode> <portfolio_filename> <start_date> <start_time> <end_time> <trend_secs> <num_past_days=90> <L1|L2>\n");
}

shortcode <- args[1];
portfolio_inputs_filename <- args[2];
start_date <- args[3];
start_time <- args[4];
end_time <- args[5];
trend_secs <- args[6];
num_days_lookback <- 90;
regularization_method <- "L2"

weight_penalty <- c(0);

#portfolio_inputs_filename <- "/spare/local/tradeinfo/PCAInfo/portfolio_inputs";

if (length(args) > 6 )
{
   num_days_lookback <- args[7];
}

if (length(args) > 7 )
{
  regularization_method <- args[8];
}

ilist_filename <- paste(work_dir,"ilist_", shortcode ,sep="");
catted_data_filename <- paste(work_dir,"catted_data_filename_", shortcode, "_", start_time, "_", end_time, "_", start_date, "_", num_days_lookback,sep="");
system( sprintf ( "rm -f %s", catted_data_filename ) );

GetListofDates();
GetPortfolioComponents();
cat(list_of_port_components,"\n");

MakeIlist();
ComputeRegData();
regdata <- as.matrix(read.table(catted_data_filename));
system( sprintf ( "rm -f %s", catted_data_filename ) );
system( sprintf ( "rm -f %s", ilist_filename ) );

Y <- regdata[,5];
X <- regdata[,6:ncol(regdata)];

stdevs <- rep(0,ncol(X));
for ( i in ( c(1:ncol(X)) ) )
{
  stdevs[i] <- sd(X[,i]);
  X[,i] = X[,i]/stdevs[i];
}

#N <- dim(X)[1];
#n_train <- floor(0.7*N);

#Y_train <- Y[1:n_train];
#X_train <- X[1:n_train,];

#X_test <- X[(n_train+1):N,];
#Y_test <- Y[(n_train+1):N];

opts <- list("algorithm"="NLOPT_LD_SLSQP");
weights <- rep ( 1/sqrt(num_port_components), num_port_components );
weights <- weights * sign_vec ;

lambda_weight <- 0 ;

OriginalCorr <- cor ( Y, X %*% weights );
best_ojective <- abs(OriginalCorr);
MaxCorrWeight <- weights;

for ( lambda_weight in weight_penalty )
{
  res <- suppressWarnings ( nloptr(x0=weights, eval_f=MaxCorr, eval_grad_f=GradNumerical, lb = LBVec, ub = UBVec, eval_g_eq = EqualityConstraint, eval_jac_g_eq = EqualityConstraintGrad, opts=opts) );
  t_weight<- rle(unlist(res[18]))$values;
  
  if ( length(t_weight) == num_port_components )
  {
    t_weight[abs(t_weight) < 0.001 ] <- 0;
    t_outsample_result <- cor ( Y, X %*% t_weight );
    cat ( "Lambda : ", lambda_weight, " Weights : ", t_weight, " Corr: ", t_outsample_result, "\n" );
    if ( abs(t_outsample_result) > abs(best_ojective) )
    {
      MaxCorrWeight <- t_weight ;
      best_ojective <- t_outsample_result
    }
  }
  else 
  {
    cat ( "Lambda : ", lambda_weight, " Weights : ", t_weight, " Degenerated to original weight", "\n" );
  }
}

cat ( "Original Correlation : ", OriginalCorr , "\n");
cat ( "Max Correlation : ", best_ojective , "\n");
cat ( "Max Correlation : Weights : ", MaxCorrWeight, "\n" );
