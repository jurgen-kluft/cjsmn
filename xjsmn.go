package main

import (
	pkg "github.com/jurgen-kluft/xjsmn/package"
)

func main() {
	xcode.Init()
	xcode.Generate(pkg.GetPackage())
}
