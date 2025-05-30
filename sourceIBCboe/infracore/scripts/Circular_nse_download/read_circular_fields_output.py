import json

with open('store_circular_data.txt') as json_file:
    data = json.load(json_file)
for row in data['data']:
    print(row['circNumber'],row['sub'],row['circFilelink'],row['circDepartment'],sep=' , ')
