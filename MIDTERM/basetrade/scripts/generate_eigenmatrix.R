#!/usr/bin/env Rscript
args <- commandArgs(trailingOnly = TRUE);

suppressPackageStartupMessages(require(glmnet))
suppressPackageStartupMessages(require(methods))

data <- read.table(args[1]);
data <- as.matrix(data);
y <- data[,1];
x <- data[,2:ncol(data)];
stdevs <- apply(x,2,sd);
x <- sweep(x,2,stdevs,FUN='/');
xtx <- t(x) %*% x;
eig <- eigen(xtx);
vec_file = paste(args[2], "vec", sep="_");
val_file = paste(args[2], "val", sep="_");
write.table(eig$vectors, file=vec_file, sep=" ", col.names = F, row.names = F);
print (eig$vectors);
print (eig$values);
eig_values <- eig$values;
pc_var_explained <- cumsum(eig_values) / sum(eig_values);
write.table(pc_var_explained, file=val_file, sep=" ", col.names = F, row.names = F);
#print(pc_var_explained);

