#!/bin/bash

CMD=$1;

echo ssh dvcinfra@sdv-chi-srv11;
echo --------------------------;
ssh dvcinfra@sdv-chi-srv11 $CMD;
echo -e "--------------------------\n";

echo ssh dvcinfra@sdv-chi-srv12;
echo --------------------------;
ssh dvcinfra@sdv-chi-srv12 $CMD;
echo -e "--------------------------\n";

echo ssh dvcinfra@sdv-chi-srv13;
echo --------------------------;
ssh dvcinfra@sdv-chi-srv13 $CMD;
echo -e "--------------------------\n";

echo ssh dvcinfra@sdv-chi-srv14;
echo --------------------------;
ssh dvcinfra@sdv-chi-srv14 $CMD;
echo -e "--------------------------\n";


echo ssh dvcinfra@sdv-fr2-srv11;
echo --------------------------;
ssh dvcinfra@sdv-fr2-srv11 $CMD;
echo -e "--------------------------\n";

echo ssh dvcinfra@sdv-fr2-srv12;
echo --------------------------;
ssh dvcinfra@sdv-fr2-srv12 $CMD;
echo -e "--------------------------\n";

echo ssh dvcinfra@sdv-fr2-srv13;
echo --------------------------;
ssh dvcinfra@sdv-fr2-srv13 $CMD;
echo -e "--------------------------\n";

echo ssh dvcinfra@sdv-fr2-srv14;
echo --------------------------;
ssh dvcinfra@sdv-fr2-srv14 $CMD;
echo -e "--------------------------\n";


echo sshtor11;
echo --------------------------;
ssh dvcinfra@38.64.128.227 $CMD;
echo -e "--------------------------\n";

echo sshtor12;
echo --------------------------;
ssh dvcinfra@38.64.128.228 $CMD;
echo -e "--------------------------\n";
