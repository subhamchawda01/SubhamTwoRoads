#grep -f /spare/local/files/LIFFE/ins_liffe.txt /spare/local/tradeinfo/PCAInfo/portfolio_inputs | awk '{print $2}' > /spare/local/files/LIFFE/all_liffe_port.txt
#cat /spare/local/files/LIFFE/ins_liffe.txt >> /spare/local/files/LIFFE/all_liffe_port.txt
grep -c -w $1 /spare/local/files/LIFFE/all_liffe_port.txt
#rm /spare/local/files/LIFFE/all_liffe_port.txt
