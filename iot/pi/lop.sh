i=1
while [ 1 ]
do
i=`expr $i + 1`
curl http://m2.exosite.com/onep:v1/stack/alias -H "X-Exosite-CIK: dc3cc6103feecbb9a2b3d450f487f747205aab31" -H "Accept: application/x-www-form-urlencoded; charset=utf-8" -d "Junk=$i"
 sleep  1
done
