if [ $# -lt 1 ]; then
 t1=1
else
 t1=$1
fi

commonDir="../Common"
pass="ForwardProp"
for i in 1 2 4 5 8 10 20; do
  . ${commonDir}/L3_ruby_CIFAR10.sh ${t1} ${t1} 1 ${i} > nohup-L3_CIFAR10_T${t1}_S${t1}_C${i}.txt & 
done
 

