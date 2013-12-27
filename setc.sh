#!/bin/bash

# setc.sh
# Setting for Client

# サーバにおいてあるソースをクライアントにコピーして
# コンパイルするまでを自動化するだけのツール

# サーバのホスト名とログインユーザ名, パスワード
. hosts

# ソースファイルのtar ballのファイル名
targz="sunfish_client.tar.gz"
dir="sunfish_client"

#Clean
rm evdata
rm gradient.gr

scp $user@$server:~/.sunfish/$targz ../

cd ..
tar zxf $targz
cd $dir
make -j
