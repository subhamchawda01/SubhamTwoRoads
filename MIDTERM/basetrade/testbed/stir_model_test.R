#! /usr/bin/env Rscript
args = commandArgs( trailingOnly=TRUE )
print(args)

tt_0 = 126
tt_1 = 378
tt_n1 = 252

n1_0_1 = read.table( args[1] )
if (length (args) > 2)
{
tt_0 = as.integer(args[2])
tt_1 = as.integer(args[3])
tt_n1 = as.integer(args[4])
}



# linear regression

fit <- lm ( n1_0_1[,2] ~ n1_0_1[,3] + n1_0_1[,4] )
summary(fit)
y = fitted(fit)
#x11()
#plot (n1_0_1[,1] ,n1_0_1[,2] , type = 'l')
#lines(n1_0_1[,1] ,y, col = "red")




#using spot/forward definition

get_n1_from_0_1_scaled <- function ( di_0 , di_1 , t_0 , t_1 , t_n1 )
{
di_0 = di_0/25200
di_1 = di_1 /25200

di_0
di_1
t_0
t_1
t_n1
temp = (1+di_0)^(t_0) * (1+di_1)^(t_1)

n_1 = (temp)^(1/(2*t_n1))
n_1 = (n_1-1)*25200
return(n_1)
}

predicted_n1 = get_n1_from_0_1_scaled( n1_0_1[,3] , n1_0_1[,4] , tt_0, tt_1, tt_n1 )
err = predicted_n1 - n1_0_1[,2]
const = mean (err)
const
#x11()
#plot (n1_0_1[,1] ,n1_0_1[,2] , type = 'l')
#lines(n1_0_1[,1] ,predicted_n1-const, col = "blue")
Sys.sleep(10);
