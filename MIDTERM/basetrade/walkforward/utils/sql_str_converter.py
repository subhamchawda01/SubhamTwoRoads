
def sql_out_to_str(output):
    output = [list(row) for row in output]
    if len(output) > 0:
        for i in range(len(output)):
            for j in range(len(output[i])):
                if type(output[i][j]) == bytearray or type(output[i][j]) == bytes:
                    output[i][j] = output[i][j].decode('utf-8')
    return output
