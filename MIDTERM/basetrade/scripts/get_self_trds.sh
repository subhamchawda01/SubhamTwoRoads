#!/bin/bash

if [ $# -lt 2 ] ; then echo "USAGE: $0 <shc> <date> [tdelta_usecs=100]" ; exit 0; fi;

td=100;
if [ $# -ge 3 ] ; then td=$3; fi;

~/basetrade_install/bin/ors_binary_reader $1 $2 | awk -vtd=$td 'BEGIN{idx=0;} { if ( $16 in SR ) {se=SR[$16]-$26;} else {se=$28;} SR[$16]=$26; if($14=="Exec"){for(x in T){ if(1000000*($10-T[x])<td) { if( Px[x]==$4 && BS[x]!=$8 && se==Sz[x] ){ print "SelfTrade:", $2, Px[x], Sz[x], T[x], $10, 1000000*($10-T[x]); } } else { delete T[x]; delete A[x]; delete Px[x]; delete BS[x]; delete Sz[x]; delete SACI[x]; } } A[idx]=$_; Px[idx]=$4; BS[idx]=$8; Sz[idx]=se; T[idx]=$10; SACI[idx]=$22; idx++; } }';
#~/basetrade_install/bin/ors_binary_reader $1 $2 | grep " ORR: Exec " | awk -vtd=$td 'BEGIN{idx=0;} { SE[$16]=$28-SE[$16]; $28=SE[$16]; for(x in T){ if(1000000*($10-T[x])<td) { if( Px[x]==$4 && BS[x]!=$8 ){ print "SelfTrade:", Px[x], Sz[x], $28, T[x], $10, 1000000*($10-T[x]); } } else { delete T[x]; delete A[x]; delete Px[x]; delete BS[x]; delete Sz[x]; } } A[idx]=$_; Px[idx]=$4; BS[idx]=$8; Sz[idx]=$28; T[idx]=$10; idx++; }';
