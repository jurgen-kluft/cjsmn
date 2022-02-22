package main

import (
	"github.com/jurgen-kluft/xcode"
	"github.com/jurgen-kluft/xhash/package"
)

func main() {
	xcode.Init()
	xcode.Generate(xhash.GetPackage())
}
