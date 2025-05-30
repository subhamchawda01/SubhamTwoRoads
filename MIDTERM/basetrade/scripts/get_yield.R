#!/usr/bin/env Rscript

args = commandArgs( trailingOnly=TRUE )

if ( length(args) < 2 ) {
	stop ("USAGE : <script> <price> <duration> [coupon=4] [frequency=2]\n");
}

Price = as.numeric(args[[1]]);
duration = as.numeric(args[[2]])/365.0;
coupon = 4
frequency = 2


if (length(args) > 2){
	coupon = as.numeric(args[[3]]);
	if (length(args) > 3){
		frequency = as.numeric(args[[4]]);
	}
}

#print(c(Price, duration, coupon, frequency));
error = function( yield) {
	Price_y = 0
	for (i in (1:(frequency * duration))){
		Price_y = Price_y + (coupon/frequency) * (1/(1 + yield/frequency))^(i)
	}

	Price_y = Price_y + (100) * (1/(1 + yield/frequency))^(frequency*duration)
	# print (c('Px ', Price_y, 'yield ', yield))
	return (( Price_y - Price) ^2);
}

output = (optim(0, error, method = "Brent", lower = -0.1, upper = 0.25))
print (output$par * 100);
#print (output$value);
