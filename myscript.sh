echo hello
ls
pwd | ls *.txt
echo pipelines test | then echo second | then echo third

echo cd and pwd test
pwd | cd subdir
then echo good | else echo not good
cd .. | pwd | ls
then echo good again | else echo not good again

echo output.txt before
echo goodbye > output.txt | cat output.txt | echo output.txt after

echo "hello world" | wc -w
cat < input.txt | grep "hello" > output.txt
echo input.txt: | cat input.txt
echo output.txt: after |cat output.txt

echo multiple redirects:
echo input: | cat input.txt
echo output: | cat output.txt
wc < input.txt > output.txt
echo output: | cat output.txt

ls *he | else echo cannot
cd subsubdir | else echo does not exit | pwd