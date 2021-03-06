//+build wsserver

package main

import (
	"./impl"
	"fmt"
	"github.com/gorilla/websocket"
	"net/http"
	"time"
)

/*
	定义一个转换器，将websocket进行封装
*/
var (
	// 定义一个转换器
	upgrader = websocket.Upgrader{
		// 握手时允许跨域 websocket服务端一般独立部署在一个域名下的，
		// 从浏览器发起一个连接到websocket服务器时,是一个跨域的
		CheckOrigin: func(r *http.Request) bool {
			return true /*return true 允许跨域*/
		},
	}
)

/*
	获取插件名称
*/
func Get_plugin_name() (pluginname string) {
	pluginname = "/wsserver"
	return
}

/*
	插件是否展示
*/
func IsDisplay() bool {
	return false
}

/*
	执行插件的功能
		1. 协议从http升级到websocket
		2. 初始化websocket
		3. 创建心跳消息go routine
		4. 进行读消息，写消息实现
*/
func Func_plugin(w http.ResponseWriter, r *http.Request) {
	var (
		wsConn *websocket.Conn
		err    error
		data   []byte
		conn   *impl.Connection
	)
	// Upgrade:websocket
	if wsConn, err = upgrader.Upgrade(w, r, nil); err != nil {
		fmt.Println("error Upgrade:websocket")
		return
	}
	if conn, err = impl.InitConnection(wsConn); err != nil {
		fmt.Println("error InitConnection")
		goto ERR
	}
	// 心跳消息
	go func() {
		var err error
		for {
			if err = conn.WriteMessage([]byte("heartbeat")); err != nil {
				fmt.Println("error heartbeat")
				return
			}
			time.Sleep(1 * time.Second)
		}
	}()
	for {
		if data, err = conn.ReadMessage(); err != nil {
			fmt.Println("error ReadMessage")
			goto ERR
		}
		if err = conn.WriteMessage(data); err != nil {
			fmt.Println("error WriteMessage")
			goto ERR
		}
	}
ERR:
	// TODO:关闭连接的操作
	conn.Close()
	return
}
