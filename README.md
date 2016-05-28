# akerclient-linux64-fix 

Latest version of Aker client for linux 64-bits avaliabe in:
http://download.aker.com.br/prod/current/index.php?path=autenticadores%2Flinux/

Only need to execute instalar.sh and follow the instrucions


The patch is in driver/ak_client_drv.c line 107

`minor = MINOR(filp->f_dentry->d_inode->i_rdev);`

Change to:

`minor = MINOR(filp->f_path.dentry->d_inode->i_rdev);`

In file driver/ak_client_hook.c line 109:

`.owner    = THIS_MODULE,`

Change to:

` #if (LINUX_VERSION_CODE < KERNEL_VERSION(4,4,0))`
`   .owner    = THIS_MODULE,`
` #endif`


In file instalar.sh, line 56:

`if  [ ${MAJOR} -ne 2 ] && [ ${MAJOR} -ne 3 ]`

Change to:

`if [ ${MAJOR} -ne 2 ] && [ ${MAJOR} -ne 3 ] && [ ${MAJOR} -ne 4 ]`




Aker Client is a product of Aker Security Solution (http://www.aker.com.br)
**this fix version are avaliabe for download AS IS without any warranties**
