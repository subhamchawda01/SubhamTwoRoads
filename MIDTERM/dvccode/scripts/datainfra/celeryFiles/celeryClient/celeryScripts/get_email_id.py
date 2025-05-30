import json

user_dict = {}

def get_email_id( user ):
    content = ""
    with open('/media/shared/ephemeral16/mapping.txt') as f:
        content = f.readlines()
    for line in content:
        line = line.split()
        if len(line) == 2:
            user_dict[line[0]] = line[1].strip()
    #print content
    if user in user_dict:
        return user_dict[user]
    else:
        return user

#print get_email_id('rj')
