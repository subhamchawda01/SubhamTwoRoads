#!/usr/bin/env Rscript

HOME_DIR <- Sys.getenv("HOME");
USER <- Sys.getenv("USER");

REPO <- "basetrade";

SCRIPTS_DIR <- paste(HOME_DIR, "/", REPO, "_install/scripts/", sep="");
MODELSCRIPTS_DIR <- paste(HOME_DIR, "/", REPO, "_install/ModelScripts/", sep="");
GENPERLLIB_DIR <- paste(HOME_DIR, "/", REPO, "_install/GenPerlLib/", sep="");
LIVE_BIN_DIR <- paste(HOME_DIR, "/LiveExec/bin/", sep="");
DATAGEN_EXEC <- paste(LIVE_BIN_DIR,"/datagen",sep="");
if ( ! file.exists(LIVE_BIN_DIR )) 
{ 
    LIVE_BIN_DIR <- paste(HOME_DIR,"/", REPO,"_install/bin/", sep="");
}

TRADELOG_DIR <- "/spare/local/logs/tradelogs/";


GetListOfDates <- function ( )
{
        script <- paste (SCRIPTS_DIR, "get_list_of_dates_for_shortcode.pl", sep="");
        dates <- system ( sprintf("%s %s %s %s 2>/dev/null",script, shortcode, start_date, num_days_lookback),intern=TRUE );
        list_of_dates <- unlist(strsplit(dates, "\\ "));
	
	for ( i in c(1:num_days_lookback) )
	{
		if ( i <= num_outsample_days ) {
			list_of_outsample_days <<- append( list_of_outsample_days, list_of_dates[i] );
		} else {
			list_of_insample_days <<- append ( list_of_insample_days, list_of_dates[i] );
		}
	}
}


GetOriginalModelCoeffs <- function ()
{
        model_text <- readLines ( model_filename );
        if ( has_siglr > 0 )
        {
                alpha <- c();
		beta <- c();
	        for ( i in c(1:length(model_text)) )
        	{
            	   if ( grepl(pattern="INDICATOR ", x = model_text[i] ) )
                   {
                	t_model_row <- unlist(strsplit(model_text[i], "\\ "));
                	t_weight <- unlist(strsplit(t_model_row[2], ":" ));
                        t_alpha <- as.numeric ( t_weight[1]);
                        t_beta <- as.numeric ( t_weight[2]);
                        alpha <- c(alpha, t_alpha );
                        beta <- c(beta, t_beta);
           	   } 
        	}        
		weights <<- c(weights, alpha);
                weights <<- c(weights, beta);
        } else {
                for ( i in c(1:length(model_text)) )
                {       
                   if ( grepl(pattern="INDICATOR ", x = model_text[i] ) )
                   {
                        t_model_row <- unlist(strsplit(model_text[i], "\\ "));
                        t_weight <- as.numeric(t_model_row[2] );
                        weights <<- c(weights,t_weight);
                   }
                }
        }
}


TestPerformance <- function ( strat_filename, num_strats, list_of_dates )
{
	sum_pnl <- rep(0,num_strats);
	sum_vol <- rep(0,num_strats);

	avg_pnl <- rep(0,num_strats);
	avg_vol <- rep(0,num_strats);

	system ( sprintf ( "> %s" , tradingdate_list_filename_ ) );
	for ( tradingdate_ in list_of_dates )
	{
		cat ( tradingdate_ , "\n" , file = tradingdate_list_filename_ , append = TRUE );

	}
        cat ("Running_sim_strat for : ", strat_filename, "\n");

	run_sim_script_ <- paste ( MODELSCRIPTS_DIR , "run_simulations_and_write_results.pl" , sep = "" );
	cat ( sprintf ( "perl %s %s %s %s %s %s\n" , run_sim_script_ , shortcode , input_strat_list_filename_ , output_results_list_filename_ , tradingdate_list_filename_ , work_dir ) );
	system ( sprintf ( "perl %s %s %s %s %s %s" , run_sim_script_ , shortcode , input_strat_list_filename_ , output_results_list_filename_ , tradingdate_list_filename_ , work_dir ) );

	for ( trading_date in list_of_dates )
	{
            for ( i in c(1:num_strat_files) )
            {
                t_strat_filename <- paste ( test_strat_filename, "_", i, sep="" );
                offset <- (i-1)*max_sim_strats_per_run;
                num_remaining <- num_strats - offset;
                if ( num_remaining < max_sim_strats_per_run )
                {
                        num_in_the_lot <- num_remaining;
                } else {
                        num_in_the_lot <- max_sim_strats_per_run;
                }

#		cat ( sprintf ( "grep \"%s %s \" %s | awk '{ print $NF; }'\n" , t_strat_filename , trading_date , output_results_list_filename_ ) );
		output_results_file_ <- system ( sprintf ( "grep \"%s %s \" %s | awk '{ print $NF; }'\n" , t_strat_filename , trading_date , output_results_list_filename_ ) , intern = TRUE , ignore.stderr = FALSE );

#		cat ( sprintf ( "cat %s\n" , output_results_file_ ) );
		sim_result_lines <- system ( sprintf ( "cat %s" , output_results_file_ ) , intern = TRUE , ignore.stderr = FALSE );

		if ( length ( sim_result_lines ) >= num_in_the_lot )
		{
		    for ( strat in c(1:num_in_the_lot) )
                    {
                     	sim_result_words <- unlist(strsplit(sim_result_lines[ strat ], "\\ " ));

                     	sum_pnl[ strat + offset ] <- sum_pnl[ strat + offset ] + as.numeric(sim_result_words[2]);
                     	sum_vol[ strat + offset ] <- sum_vol[ strat + offset ] + as.numeric(sim_result_words[3]);
                    }
		}
	    }
	}

	for ( i in c(1:num_strats) )
	{
		avg_pnl[i] <- sum_pnl[i]/length(list_of_dates);
		avg_vol[i] <- sum_vol[i]/length(list_of_dates);		
	}
		
	max_pnl <- -100000;
	max_vol <- 0;
	max_score <- -1000000000;
	
	max_index <- 0;

	for ( i in c(1:num_strats) )
	{
          if ( avg_vol[i] > 0 )
          {
	    if ( avg_pnl[i] > 0 ) 
	    {
	       t_score <- avg_pnl[i] * sqrt ( avg_vol[i] );		
	    } else 
	    {
	       t_score <- avg_pnl[i] / sqrt ( avg_vol[i] );
	    }
          } else 
          {
            t_score <- -Inf ;
          }
     
	    cat ( "For Strategy ",i, " Avg Pnl : ",floor(avg_pnl[i]), " Avg Vol : ", floor(avg_vol[i]), " Avg Score : ", t_score, "\n", file = results_filename, append=TRUE );


            if (length(best_cost_vec) == 0){
                best_cost_vec <<- append(best_cost_vec, t_score);
                best_index_vec <<- append(best_index_vec, i);
            } else {
                for (index in 1:length(best_cost_vec))
                {
                  if (length(best_cost_vec) < max_models_to_keep){
                    if(t_score > best_cost_vec[index]){
                        best_cost_vec <<- append(best_cost_vec,t_score,index-1);
                        best_index_vec <<- append(best_index_vec, i, index-1);
                        break;
                    }

                    if(index==length(best_cost_vec)){
                        best_cost_vec <<- append(best_cost_vec,t_score,index);
                        best_index_vec <<- append(best_index_vec, i, index);
                        break;
                    }
                  } else {
                    if(t_score > best_cost_vec[index]){
                        best_cost_vec <<- append(best_cost_vec,t_score,index-1);
                        best_index_vec <<- append(best_index_vec, i, index-1);
                        length(best_cost_vec) <<- max_models_to_keep ;
                        length(best_index_vec) <<- max_models_to_keep ;
                        break;
                    }
                  }
                 }
             }


	    if ( t_score > max_score )
	    {
		max_score <- t_score;
		max_pnl <- avg_pnl[i];
		max_vol <- avg_vol[i];
		max_index <- i; 
	    }
	}
	best_indicator_index <<- max_index;
}

WriteOutModel <- function ( ) 
{
	t_model_size <- as.numeric ( system ( sprintf ( "cat %s | grep \"INDICATOR\" | wc -l", model_filename ), intern=TRUE ) );
	system ( sprintf ( "cat %s | awk '{if(NR==%s){$2=%s} print $0}' > %s", model_filename, t_model_size + 1, weight_new, new_model_filename ) );
}

BuildNewModels <- function ()
{
	ilist_text <- readLines ( ilist );
	model_text <- readLines ( model_filename );
	
	for ( i in c(1:num_indicators) )
	{
		t_x <- X_new[,i];
		num_models <<- num_models + 1;
		t_new_model_filename <- paste (work_dir, "tmp_model_", num_models,sep="");
		indep_corrs <- cor ( t_x, X_orig );

		indep_corrs [ which(is.na(indep_corrs)) ] <- 0;

		if ( sum (abs(indep_corrs) > max_indep_correlation ) >=1 )
		{
		    num_models <<- num_models - 1;
		    next;
		}

		res_corr <- cor( Y_res, t_x );
		if( is.na(res_corr) ) {
			res_corr <- 0;
		}

		dep_corr <- cor ( Y, t_x );
		
		if( is.na(dep_corr) ) {
			dep_corr <- 0;
		}

		if ( res_corr * dep_corr < 0 )
		{
		    num_models <<- num_models - 1;
		    next;
		}

		orig_model_correlation <- cor( Y, Y_hat_orig );

                if ( has_siglr > 0 )
                {
             	    t_x <- 10000 * t_x;   # high alpha to make the relationship linear
                    t_x <- 1/(1 + exp(-t_x)) - 0.5;
                }

                weight_new <- sum ( Y_res * t_x ) / sum ( t_x * t_x );

                Y_hat <- Y_hat_orig + t_x * weight_new ;
                 

		mse_orig <- sqrt(sum((Y - Y_hat_orig ) * ( Y - Y_hat_orig ))/length(Y_hat_orig));
		mse_final <- sqrt(sum((Y - Y_hat) * (Y - Y_hat))/length(Y_hat));

		t_ilist_row <- unlist(strsplit(ilist_text[i+3], "\\ "));
                t_ilist_row[2] <- weight_new;

                if ( has_siglr > 0 )
		{            
		   t_ilist_row[2]  <- sprintf ( "10000:%s", weight_new );
                }
                
	
		for ( j in c(1:length(model_text) ) )
		{
			if ( j==1 )
			{
			    cat ( model_text[j],"\n",file=t_new_model_filename );
			} else {
			    cat ( model_text[j],"\n",file=t_new_model_filename, append=TRUE );			
			    if ( j == length (model_text) - 1)
			    {
			        cat ( t_ilist_row,"\n",file=t_new_model_filename, append=TRUE );
			    }
			}
		}				
	
		cat ("For indicator " , i , " mse : ", mse_orig, " ", mse_final, "\n", file = results_filename, append = TRUE );	
	}	
}

BuildCattedStratFile <- function()
{
	strat_text <- readLines ( strat_file );
	t_strat_row <- unlist ( strsplit( strat_text, "\\ " ) );
	
	last_test_strat_file = "";

	for ( i in c(1:num_models) )
	{
                test_strat_file_index <- ceiling ( i / max_sim_strats_per_run );
                test_strat_file <- paste ( test_strat_filename, "_", test_strat_file_index, sep="" );
		system ( sprintf ( "> %s" , test_strat_file ) );
	}

	for ( i in c(1:num_models) )
	{
                test_strat_file_index <- ceiling ( i / max_sim_strats_per_run );
                test_strat_file <- paste ( test_strat_filename, "_", test_strat_file_index, sep="" );
                t_strat_row[4] <- paste ( work_dir, "/tmp_model_", i, sep="" );
                t_strat_row[length(t_strat_row)] <- as.numeric(t_strat_row[length(t_strat_row)]) + 1;
		cat ( t_strat_row,"\n", file=test_strat_file, append=TRUE );

		if ( test_strat_file != last_test_strat_file )
		{
			cat ( test_strat_file , "\n" , file = input_strat_list_filename_ , append = TRUE );
			last_test_strat_file = test_strat_file;
		}
	}
}

GetDataForRegression <- function ()
{
	system ( sprintf ( "> %s" , tradingdate_list_filename_ ) );
	for ( tradingdate_ in list_of_insample_days )
	{
		cat ( tradingdate_ , "\n" , file = tradingdate_list_filename_ , append = TRUE );
	}

	run_datagen_script <- paste ( MODELSCRIPTS_DIR , "run_datagen_and_generate_reg_data_kkp.pl" , sep = "" );
	cat ( sprintf ( "perl %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s\n" ,
	      	        run_datagen_script , shortcode , model_plus_ilist_file , tradingdate_list_filename_ ,
			start_time , end_time , dgen_msecs , dgen_l1events , dgen_trades , to_print_on_eco ,
			pred_duration , pred_algo , catted_regdata_filename , work_dir , sampling_shortcode_str_, datagen_exec_ ) );
	system ( sprintf ( "perl %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s\n" ,
	      	        run_datagen_script , shortcode , model_plus_ilist_file , tradingdate_list_filename_ ,
			start_time , end_time , dgen_msecs , dgen_l1events , dgen_trades , to_print_on_eco ,
			pred_duration , pred_algo , catted_regdata_filename , work_dir , sampling_shortcode_str_, datagen_exec_ ) );

	apply_dep_filter_script <- paste(MODELSCRIPTS_DIR, "apply_dep_filter.pl", sep="");

        system ( sprintf ( "%s %s %s %s %s %s", apply_dep_filter_script, shortcode, catted_regdata_filename, filter, filtered_regdata_filename, start_date ) );

}


args = commandArgs( trailingOnly=TRUE )
if ( length(args) < 18 ) {
        stop ("USAGE : <script> <shortcode> <strat_file> <model_file> <ilist> <start_date> <num_days_lookback> <start_time> <end_time> <dgen_msecs> <dgen_l1events> <dgen_trades> <to_print_on_eco> <pred_duration> <pred_algo> <filter> <work_dir> <output_aim_file> <sampling_shortcode_string>\n");
}
cat (length(args),"\n");
num_models <- 0;
shortcode <- args[1];
strat_file <- args[2];
model_filename <- args[3];
ilist <- args[4];
start_date <- args[5];
num_days_lookback <- as.numeric(args[6]);
start_time <- args[7];
end_time <- args[8];
dgen_msecs <- args[9];
dgen_l1events <- args[10];
dgen_trades <- args[11];
to_print_on_eco <- as.numeric(args[12]);
pred_duration <- as.numeric(args[13]);
pred_algo <- args[14];
filter <- args[15];
work_dir <- args[16];
aim_model_file_list <- args[17];
sampling_shortcode_ <- c();
datagen_exec_ <- DATAGEN_EXEC;
if (length(args)>18)
{
    sampling_shortcode_[1] <- args[18];
    sampling_shortcode_str_ <- paste(sampling_shortcode_, collapse = " ");
} else {
    sampling_shortcode_str_ <- "";
}
if (length(args)>19)
{
datagen_exec_ <- args[19];
}

cat (sampling_shortcode_str_, "\n");
#work_dir <- paste("/spare/local/",system ("whoami",intern=TRUE), "/", shortcode, "/", floor(runif(1,100000,999999)),sep="");
system ( sprintf ("mkdir -p %s", work_dir) );
catted_regdata_filename <- paste( work_dir, "catted_regdata_outfile", sep="" );
filtered_regdata_filename <- paste(work_dir, "filtered_regdata_filename", sep="");
weights <- c();
best_indicator_index <- 0;
new_model_filename <- paste( work_dir, "new_model_file",sep="" );
results_filename <- paste( work_dir, "results", sep="" );

max_sim_strats_per_run <- 50;

num_outsample_days <- 0.001 * num_days_lookback;

list_of_insample_days <- c();
list_of_outsample_days <- c(); 

max_models_to_keep <- 5 ;
best_cost_vec <- c() ;
best_index_vec <- c() ;

max_indep_correlation <- 0.7;

has_siglr <- as.numeric(system( sprintf( "cat %s | grep \"SIGLR \" | wc -l", model_filename ), intern=TRUE ));

GetListOfDates();
GetOriginalModelCoeffs();

model_plus_ilist_file <- paste( work_dir, "model_plus_ilist_file", sep="" );

temp_model_file <- paste( work_dir, "tmp_model_file_tmp", sep="" );

system ( sprintf ( "cp %s %s", model_filename, temp_model_file ) );

system ( sprintf ( "cat %s | grep -v INDICATOREND > %s", temp_model_file, model_plus_ilist_file ) );

system ( sprintf ( "cat %s | grep \"INDICATOR \" >> %s", ilist, model_plus_ilist_file ) );

system ( sprintf ( "echo INDICATOREND >> %s", model_plus_ilist_file ) );

num_model_file_indicators <- as.numeric(system( sprintf( "cat %s | grep \"INDICATOR \" | wc -l", model_filename ), intern=TRUE ) );

cat ( "num_model_file_indicators ", num_model_file_indicators, "\n" );

system ( sprintf( "~/basetrade/scripts/remove_dup_indicators_from_ilist.pl %s ", model_plus_ilist_file ) );

num_model_plus_ilist_indicators <-  as.numeric(system( sprintf( "cat %s | grep \"INDICATOR \" | wc -l", model_plus_ilist_file ), intern=TRUE ) );

cat ( "num_model_plus_ilist_indicators ", num_model_plus_ilist_indicators , "\n" );

num_indicators <- num_model_plus_ilist_indicators - num_model_file_indicators;

cat ( "num_indicators ", num_indicators, "\n" );

tradingdate_list_filename_ <- paste ( work_dir , "tradingdate_list_file" , sep="" );

GetDataForRegression ();
regdata <- as.matrix(read.table(filtered_regdata_filename));

Y <- regdata[,1];
X_orig <- regdata[,2:(ncol(regdata) - num_indicators)];

X_new <- regdata[,( ncol(regdata) - num_indicators + 1 ): ncol(regdata) ];


test_strat_filename <- paste( work_dir, "test_strat_file", sep="" );

if ( has_siglr > 0 )
{
  t_alpha <- weights[1:num_model_file_indicators];
  t_beta <- weights[(num_model_file_indicators+1):(2*num_model_file_indicators)];
  T <- X_orig * t(replicate(nrow(X_orig),t_alpha));
  H <- 1/(1 + exp(-T)) - 0.5;
  Y_hat_orig <- H %*% t_beta;
  Y_res <- Y - Y_hat_orig;
} else {
  Y_hat_orig <- X_orig %*% weights;
  Y_res <- Y - Y_hat_orig;
}

BuildNewModels ();

if ( num_models <= 0 )
{
  cat ("ERROR: NO MODELS CREATED\n" );
  exit(1);
}

input_strat_list_filename_ <- paste ( work_dir , "input_catted_strat_list_file" , sep = "" );
system ( sprintf ( "> %s" , input_strat_list_filename_ ) );
output_results_list_filename_ <- paste ( work_dir , "output_results_list_file" , sep = "" );
system ( sprintf ( "> %s" , output_results_list_filename_ ) );

BuildCattedStratFile ();

num_strat_files <- ceiling ( num_models/max_sim_strats_per_run);

#cat ("Original Strategy File Insample Results\n", file = results_filename, append = TRUE );

#TestPerformance ( strat_file, 1, list_of_insample_days );

cat ("Modified Strategy Files Insample Results\n", file = results_filename, append = TRUE );

TestPerformance  ( test_strat_filename, num_models, list_of_insample_days );


cat ( "Best Indicator Index ", best_indicator_index, "\n", file = results_filename, append = TRUE );

#final_model_file <- paste ( work_dir, "/tmp_model_", best_indicator_index , sep="" );

for ( i in c(1:length(best_index_vec) ) )
{
  cat ("Best Index: ", best_index_vec[i], "\n");
  temp_model_file <- paste ( work_dir, "tmp_model_", best_index_vec[i] , sep="" );
  new_model <- paste ( work_dir, "new_model_", best_index_vec[i] , sep="" );
  system ( sprintf ("cp %s %s", temp_model_file, new_model ) );
  system ( sprintf ("echo %s >> %s", new_model, aim_model_file_list) );
}

#system ( sprintf ("cp %s %s", final_model_file, new_model_filename ) );

#for ( i in c(1:num_models) )
#{
#	t_model_file <- paste ( work_dir, "/tmp_model_", i , sep="" );
#	system ( sprintf ( "rm -f %s", t_model_file ) );
#}

system ( sprintf ( "rm -f %s ", filtered_regdata_filename ) );
system ( sprintf ( "rm -f %s ", catted_regdata_filename ) );
