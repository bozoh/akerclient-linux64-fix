# akerclient-linux64-fix 

Fix version of Aker client for linux 64-bits avaliabe in:
http://download.aker.com.br/prod/current/index.php?path=autenticadores%2Flinux/

Only need to execute instalar.sh and follow the instrucions


Aker Client is a product of Aker Security Solution (http://www.aker.com.br)
this fix version are avaliabe for download AS IS without any warranties


The patch is in driver/ak_client_drv.c line 107

minor = MINOR(filp->f_dentry->d_inode->i_rdev);

change to:

minor = MINOR(filp->f_path.dentry->d_inode->i_rdev);
