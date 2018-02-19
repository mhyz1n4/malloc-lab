# -*- mode: ruby -*-
# vi: set ft=ruby :

# Vagrantfile API/syntax version. Don't touch unless you know what you're doing!
VAGRANTFILE_API_VERSION = "2"

Vagrant.configure(VAGRANTFILE_API_VERSION) do |config|
  config.vm.box = "ubuntu/xenial64"
  #config.vm.box_url = "https://vagrantcloud.com/ubuntu/boxes/xenial64/versions/20180105.0.0/providers/virtualbox.box"

  config.vm.provider :virtualbox do |vb|
    vb.cpus = 1
    vb.memory = 512
  end

  # Automatically use local apt-cacher-ng if available
  if File.exists? "/etc/apt-cacher-ng"
    # If apt-cacher-ng is installed on this machine then just use it.
    require 'socket'
    guessed_address = Socket.ip_address_list.detect{|intf| !intf.ipv4_loopback?}
    if guessed_address
      config.vm.provision :shell, :inline => "echo 'Acquire::http { Proxy \"http://#{guessed_address.ip_address}:3142\"; };' > /etc/apt/apt.conf.d/02proxy"
    end
  end

  # Fix: ttyname failed: Inappropriate ioctl for device
  # Link: https://superuser.com/questions/1160025/how-to-solve-ttyname-failed-inappropriate-ioctl-for-device-in-vagrant
  config.ssh.shell = "bash -c 'BASH_ENV=/etc/profile exec bash'"

  # Update to have the latest packages, remove if you don't need that
  config.vm.provision :shell, :inline => "apt-get update"
  config.vm.provision :shell, :inline => "apt-get install -y build-essential gcc-multilib emacs htop gdb"

  # For all users
  config.vm.provision :shell, :inline => "for i in `ls /home`; do echo 'cd /vagrant' >>/home/${i}/.bashrc; done"
  config.vm.provision :shell, :inline => "for i in `ls /home`; do curl http://www.cse.psu.edu/~tuz68/.emacs 2>/dev/null >/home/${i}/.emacs; chown ${i}:${i} /home/${i}/.emacs; done"
  config.vm.provision :shell, :inline => "for i in `ls /home`; do curl https://raw.githubusercontent.com/mitthu/cmpsc473_spring18/master/base/dot_vimrc 2>/dev/null >/home/${i}/.vimrc; chown ${i}:${i} /home/${i}/.vimrc; done"
end
