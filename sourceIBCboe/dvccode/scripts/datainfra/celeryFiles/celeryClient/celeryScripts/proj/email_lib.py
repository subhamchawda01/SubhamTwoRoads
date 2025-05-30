import smtplib
from smtplib import SMTPException

def email_task(prog, output, ret, user, hostname="default"):
    sender = 'nseall@tworoads.co.in'
    receivers = [ user ]
    subject = "Distributed Setup: " + prog
    #print prog, output, ret
    text = "Hostname: " + hostname + "\nProgram: " + prog + "\nOutput: " + output + "\nReturn Code: " + str(ret)

    message = """\
From: %s
To: %s
Subject: %s

%s
""" % (sender, ", ".join(receivers), subject, text)
    try:
        smtpObj = smtplib.SMTP('localhost')
        smtpObj.sendmail(sender, receivers, message)         
    except SMTPException as err:
        raise err

def prog_output(given_input):
    print(given_input)
    [prog, user, output_file, output, err, ret, time_duration] = given_input
    text = "Program: " + prog + "\nOutput: " + output + "\nErr:" + str(err) + "\nReturn Code: " + str(ret) + "\nTime Duration" + str(time_duration)
    return text
