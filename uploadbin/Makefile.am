bin_PROGRAMS = uploadutil_test

uploadutil_test_SOURCES = main_upload_test.c
                        
uploadutil_test_CFLAGS = -I${top_srcdir}/dwnlutils -I$(top_srcdir)/utils -I${top_srcdir}/uploadutils -DLIBRDKCERTSELECTOR
uploadutil_test_LDADD = -lrdkloggers -lpthread $(curl_LIBS)  -L$(PKG_CONFIG_SYSROOT_DIR)/usr/lib -lRdkCertSelector -luploadutil ldwnlutil -lfwutils  -l -L$(PKG_CONFIG_SYSROOT_DIR)/usr/lib64 -lrdkconfig
#uploadutil_test_LDADD += ${top_builddir}/dwnlutils/libdwnlutil.la ${top_builddir}/utils/libfwutils.la ${top_builddir}/uploadutils/libuploadutil.la
