___
echo server
___

- key function
	pipe 
	splice
	tee

- procedure
	1. create pipe file
	2. utilize splice re-direct the original fd to pipe fd one[1]
	3. copy pipe fd one[0] as new pipe fd[0] by tee
	4. utilize splice re-direct the new pipe fd[1] to the original fd