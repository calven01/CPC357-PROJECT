obj = parameter[1]["data"]
arrlgt = parameter[1]["count"]["total"] - 1
gas = obj[arrlgt]["Gas detector"]
threshold = 2000 #change with average

if int(gas) > threshold:
    msgbody='<p>Emergency! There is gas leak in your house.</p><br>'
    output[1]="[Warning] Gas Reading Over Threshold "
    output[2]=msgbody
    output[3]=2
