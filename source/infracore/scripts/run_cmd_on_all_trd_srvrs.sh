#!/bin/bash

CMD=$1;

echo ssh sdv-ny4-srv11;
echo --------------------------;
ssh sdv-ny4-srv11 $CMD;
echo -e "--------------------------\n";

echo ssh sdv-ny4-srv12;
echo --------------------------;
ssh sdv-ny4-srv12 $CMD;
echo -e "--------------------------\n";

echo ssh sdv-ny4-srv13;
echo --------------------------;
ssh sdv-ny4-srv13 $CMD;
echo -e "--------------------------\n";

echo ssh sdv-ny4-srv14;
echo --------------------------;
ssh sdv-ny4-srv14 $CMD;
echo -e "--------------------------\n";

echo ssh sdv-ny4-srv15;
echo --------------------------;
ssh sdv-ny4-srv15 $CMD;
echo -e "--------------------------\n";

echo ssh sdv-ny4-srv16;
echo --------------------------;
ssh sdv-ny4-srv16 $CMD;
echo -e "--------------------------\n";

echo ssh sdv-chi-srv11;
echo --------------------------;
ssh sdv-chi-srv11 $CMD;
echo -e "--------------------------\n";

echo ssh sdv-chi-srv12;
echo --------------------------;
ssh sdv-chi-srv12 $CMD;
echo -e "--------------------------\n";

echo ssh sdv-chi-srv13;
echo --------------------------;
ssh sdv-chi-srv13 $CMD;
echo -e "--------------------------\n";

echo ssh sdv-chi-srv14;
echo --------------------------;
ssh sdv-chi-srv14 $CMD;
echo -e "--------------------------\n";


echo ssh sdv-fr2-srv11;
echo --------------------------;
ssh 10.23.200.51 $CMD;
echo -e "--------------------------\n";

echo ssh sdv-fr2-srv12;
echo --------------------------;
ssh 10.23.200.52 $CMD;
echo -e "--------------------------\n";

echo ssh sdv-fr2-srv13;
echo --------------------------;
ssh 10.23.200.53 $CMD;
echo -e "--------------------------\n";

echo ssh sdv-fr2-srv14;
echo --------------------------;
ssh 10.23.200.54 $CMD;
echo -e "--------------------------\n";


echo sshtor11;
echo --------------------------;
ssh sdv-tor-srv11 $CMD;
echo -e "--------------------------\n";

echo sshtor12;
echo --------------------------;
ssh sdv-tor-srv12 $CMD;
echo -e "--------------------------\n";

echo sshbmf11;
echo --------------------------;
ssh sdv-bmf-srv11 $CMD;
echo -e "--------------------------\n";

echo sshbmf12;
echo --------------------------;
ssh sdv-bmf-srv12 $CMD;
echo -e "--------------------------\n";

echo sshbmf13;
echo --------------------------;
ssh sdv-bmf-srv13 $CMD;
echo -e "--------------------------\n";
