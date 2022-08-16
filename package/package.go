package xjsmn

import (
	"github.com/jurgen-kluft/ccode/denv"
	"github.com/jurgen-kluft/xbase/package"
	"github.com/jurgen-kluft/xentry/package"
)

// GetPackage returns the package object of 'xjsmn'
func GetPackage() *denv.Package {
	// Dependencies
	xunittestpkg := xunittest.GetPackage()
	xentrypkg := xentry.GetPackage()
	xbasepkg := xbase.GetPackage()

	// The main (xjsmn) package
	mainpkg := denv.NewPackage("xjsmn")
	mainpkg.AddPackage(xunittestpkg)
	mainpkg.AddPackage(xentrypkg)
	mainpkg.AddPackage(xbasepkg)

	// 'xjsmn' library
	mainlib := denv.SetupDefaultCppLibProject("xjsmn", "github.com\\jurgen-kluft\\xjsmn")
	mainlib.Dependencies = append(mainlib.Dependencies, xbasepkg.GetMainLib())

	// 'xjsmn' unittest project
	maintest := denv.SetupDefaultCppTestProject("xjsmn_test", "github.com\\jurgen-kluft\\xjsmn")
	maintest.Dependencies = append(maintest.Dependencies, xunittestpkg.GetMainLib())
	maintest.Dependencies = append(maintest.Dependencies, xentrypkg.GetMainLib())
	maintest.Dependencies = append(maintest.Dependencies, xbasepkg.GetMainLib())
	maintest.Dependencies = append(maintest.Dependencies, mainlib)

	mainpkg.AddMainLib(mainlib)
	mainpkg.AddUnittest(maintest)
	return mainpkg
}
