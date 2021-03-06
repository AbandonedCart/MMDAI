require File.dirname(__FILE__) + '/base.rb'

module Mmdai

  module Build

    # included class must implement below methods
    # * get_build_options
    # * get_directory_name
    module CMake
      include Base

    protected
      def start_build(build_options, extra_options)
        cmake = get_cmake build_options, extra_options
        inside get_build_path do
          run cmake
          run_build
          run_build "install"
        end
      end

      def start_clean(arch = false)
        inside get_build_path do
          run_build "clean"
          FileUtils.rmtree [
            'CMakeCache.txt',
            'CMakeFiles',
            'cmake_install.cmake',
            'install_manifest.txt',
            'Makefile',
            INSTALL_ROOT_DIR
          ]
        end
      end

      def get_cmake_executable()
        return find_env_vars [ "VPVL2_CMAKE_EXECUTABLE", "CMAKE_EXECUTABLE" ], "cmake"
      end

      def get_cmake(build_options, extra_options)
        cmake = get_cmake_executable
        cmake += " "
        build_path = get_build_path
        build_type = get_build_type
        build_options.merge!({
          :build_shared_libs => will_built_as_dll?(build_options, extra_options),
          :cmake_build_type => (is_debug_build?(extra_options) ? "Debug" : "Release"),
          :cmake_install_prefix => "#{build_path}/#{INSTALL_ROOT_DIR}",
          :cmake_install_name_dir => "#{build_path}/#{INSTALL_ROOT_DIR}/lib",
        })
        if build_type === :release and not is_msvc? then
          add_cxx_flags "-fvisibility=hidden -fvisibility-inlines-hidden", build_options
        elsif build_type === :flascc then
          add_cc_flags "-fno-rtti -O4", build_options
        elsif build_type === :emscripten then
          emscripten_path = ENV['EMSCRIPTEN']
          cmake = "#{emscripten_path}/emconfigure cmake -DCMAKE_AR=#{emscripten_path}/emar "
        elsif is_executable? then
          build_options.delete :build_shared_libs
        else
          build_options[:library_output_path] = "#{build_path}/lib"
        end
        if is_darwin? then
          add_cc_flags "-mmacosx-version-min=" + (build_options[:cmake_osx_deployment_target] || "10.6"), build_options
          build_options.delete :cmake_osx_deployment_target
        end
        envvars = [
          "ANDROID_NATIVE_API_LEVEL",
          "ANDROID_NDK",
          "ANDROID_TOOLCHAIN_NAME",
          "CMAKE_TOOLCHAIN_FILE",
          "IOS_PLATFORM"
        ]
        envvars.each do |key|
          if ENV.key? key then
            build_options[key.downcase.to_sym] = ENV[key]
          end
        end
        return serialize_build_options cmake, build_options, extra_options.key?(:printable)
      end

      def serialize_build_options(cmake, build_options, is_printable)
        build_options.each do |key, value|
          cmake += "-D"
          cmake += key.to_s.upcase
          if !!value == value then
          cmake += ":BOOL="
            cmake += value ? "ON" : "OFF"
          else
            cmake += ":STRING='"
            cmake += value
            cmake += "'"
          end
          cmake += is_printable ? " \\\n" : " "
        end
        build_type = get_build_type
        if is_ninja? then
          cmake += "-G Ninja "
        elsif build_type === :vs2012 then
          cmake += "-G \"Visual Studio 11\" "
        elsif build_type === :vs2010 then
          cmake += "-G \"Visual Studio 10\" "
        elsif build_type === :ios then
          cmake += "-G Xcode "
        end
        cmake += ".."
        return cmake
      end

      def print_build_options(extra_options = {})
        extra_options[:printable] = true
        puts get_cmake get_build_options(get_build_type, extra_options), extra_options
      end

      def add_cc_flags(cflags, build_options)
        build_options[:cmake_c_flags] ||= ""
        build_options[:cmake_c_flags] += cflags + " "
        add_cxx_flags(cflags, build_options)
      end

      def add_cxx_flags(cflags, build_options)
        build_options[:cmake_cxx_flags] ||= ""
        build_options[:cmake_cxx_flags] += cflags + " "
      end

      def is_debug_build?(extra_options = {})
        return get_build_type === :debug && !extra_options.key?(:force_release)
      end

      def will_built_as_dll?(build_options = {}, extra_options = {})
        return !build_options.key?(:build_shared_libs) && is_debug_build?(extra_options) && !is_msvc? && get_build_type != :ios
      end

    end

  end

end
