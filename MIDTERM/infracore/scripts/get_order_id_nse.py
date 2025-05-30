import struct
import binascii
import sys

def ChangeEndian(s_hex):
    rev_s = ""
    i = len(s_hex) - 2
    while ( i >= 0 ):
        rev_s += s_hex[i]
        rev_s += s_hex[i+1]
        i = i -2
    return rev_s

if ( len(sys.argv) < 2):
    print ("Usage : <script> <Internal_Order_ID>")
    sys.exit(1)

order_id = int(sys.argv[1])

hex_str = hex(order_id)[2:]

hex_str_endian_changed = ChangeEndian(hex_str)

nse_order_id = struct.unpack('d', binascii.unhexlify(hex_str_endian_changed))[0]

print ("NSE_Order_ID: " + str(nse_order_id))



