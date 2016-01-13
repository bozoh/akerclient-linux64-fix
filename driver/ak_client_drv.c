/*!
 * \file ak_client_drv.c
 * \brief Modulo de controle do device do Aker Client.
 * \date 05/09/2007
 * \author Rodrigo Ormonde
 * \ingroup akerclient_driver_linux
 *
 * Copyright (c)1997-2007 Aker Security Solutions
 *
 * $Id: ak_client_drv.c,v 1.9.2.1 2012/07/27 13:49:35 neuton.martins Exp $
 */

#include <linux/slab.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/types.h>
#include <linux/fs.h>
#include <linux/in.h>
#include <linux/cdev.h>
#include <asm/uaccess.h>

#include "aktypes.h"
#include "ak_client_drv.h"

// Prototipo das funcoes locais

static int ak_client_open(struct inode *inode, struct file *filp);
static int ak_client_close(struct inode *inode, struct file *filp);
static long ak_client_ioctl(struct file *filp, unsigned int cmd, unsigned long arg);

// Variaveis globais

#ifdef DECLARE_MUTEX
DECLARE_MUTEX(aker_client_mutex);
#else
DEFINE_SEMAPHORE(aker_client_mutex);
#endif
static int ak_client_dev_open = 0;	// Device ja foi aberto ?
static dev_t ak_drv_dev;		// Numero do device (major e minor)
static struct cdev ak_client_cdev;
static struct file_operations ak_client_fops = {
  .owner = THIS_MODULE,
  .open = ak_client_open,
  .release = ak_client_close,
  .unlocked_ioctl = ak_client_ioctl
};


/*! Esta funcao e' chamada na abertura do device do Aker Client */

static int ak_client_open(struct inode *inode, struct file *filp)
{
  int minor;			/* Numero minor do device */
  int error = 0;

  if (down_interruptible(&aker_client_mutex))	// Faz lock do semaforo
    return -ERESTARTSYS;

  minor = MINOR(inode->i_rdev);
  if (minor == AK_CLIENT_API_MINOR)
  {
    if (ak_client_dev_open)
      error = -EBUSY;
    else
      ak_client_dev_open = 1;
  }
  else
    error = -ENXIO;

  up(&aker_client_mutex);		// Libera semaforo
  return error;
}


/*! Esta funcao e' chamada no fechamento do devices do Aker Client */

static int ak_client_close(struct inode *inode, struct file *filp)
{
  int minor;			/* Numero minor do device */
  int error = 0;

  if (down_interruptible(&aker_client_mutex))	// Faz lock do semaforo
    return -ERESTARTSYS;

  minor = MINOR(inode->i_rdev);
  if (minor == AK_CLIENT_API_MINOR)
    ak_client_dev_open = 0;
  else
    error = -ENXIO;

  up(&aker_client_mutex);		// Libera semaforo
  return error;
}


/*! Esta funcao intercepta as operacoes de ioctl do device aker_client */

static long ak_client_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{
  int size = _IOC_SIZE(cmd);	// Bloco de dados a ler/gravar
  int minor;			// Numero minor do device
  void *buffer;
  int error;

  if (_IOC_TYPE(cmd) != 'A')
    return -ENOTTY;
  minor = MINOR(filp->f_path.dentry->d_inode->i_rdev);
  if (minor != AK_CLIENT_API_MINOR)
    return -ENODEV;

  switch(cmd)
  {
    case AK_CLIENT_LOGIN:
      if (size != sizeof(ak_client_logon))
        return -EINVAL;

      buffer = kmalloc(size, GFP_KERNEL);
      if (!buffer)
        return -ENOMEM;

      if (copy_from_user(buffer, (void *) arg, size))
      {
        kfree(buffer);
        return -EFAULT;
      }
      error = 0 - ak_client_add_user((ak_client_logon *) buffer);
      kfree(buffer);
      return error;
      break;

    case AK_CLIENT_LOGOUT:
      if (size != sizeof(ak_client_logoff))
        return -EINVAL;

      buffer = kmalloc(size, GFP_KERNEL);
      if (!buffer)
        return -ENOMEM;

      if (copy_from_user(buffer, (void *) arg, size))
      {
        kfree(buffer);
        return -EFAULT;
      }
      error = 0 - ak_client_remove_user((ak_client_logoff *) buffer);
      kfree(buffer);
      return error;
      break;

    case AK_CLIENT_FLUSH:
      ak_client_flush_users();
      return 0;
      break;

    case AK_CLIENT_UDP_ENABLE:
      if (size != sizeof(int))
        return -EINVAL;
      ak_client_udp_enable((int) arg);
      return 0;
      break;

    default:
      return -ENOTTY;		// Comando invalido
      break;
  }
}


/*! Esta funcao inicializa o driver e o hook de pacotes. Ela e' chamada quando
    o modulo e' carregado */

static int ak_client_init(void)
{
  int error;

  // Aloca um numero de device dinamicamente

  error = alloc_chrdev_region(&ak_drv_dev, 0, 1, AK_CLIENT_NAME);
  if (error)
    return error;

  // Cria um novo device, com o numero ja alocado

  cdev_init(&ak_client_cdev, &ak_client_fops);
  ak_client_cdev.owner = THIS_MODULE;
  ak_client_cdev.ops = &ak_client_fops;
  error = cdev_add(&ak_client_cdev, ak_drv_dev, 1);
  if (error)
  {
    unregister_chrdev_region(ak_drv_dev, 1);
    return error;
  }

  if (ak_client_register_hook())
  {
    cdev_del(&ak_client_cdev);
    unregister_chrdev_region(ak_drv_dev, 1);
    return -EACCES;
  }
  printk("Aker Client module loaded\n");

  return 0;
}


/*! Esta funcao remove o modulo do kernel */

static void ak_client_exit(void)
{
  ak_client_unregister_hook();
  cdev_del(&ak_client_cdev);
  unregister_chrdev_region(ak_drv_dev, 1);

  printk("Aker Client module unloaded\n");
}

MODULE_LICENSE("GPL");
module_init(ak_client_init);
module_exit(ak_client_exit);
