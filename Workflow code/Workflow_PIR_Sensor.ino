obj = parameter[1]["data"]
arrlgt = parameter[1]["count"]["total"] - 1
motion = obj[arrlgt]["Motion"]
threshold = 0.4034334763948498 #replaced with average value

if int(motion) > threshold:
    msgbody='<p> Emergency, There is movement inside your living room.</p><br>'
    output[1]="[Alert]  Motion Detected! "
    output[2]=msgbody
    output[3]=2
