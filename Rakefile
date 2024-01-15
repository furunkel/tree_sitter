# frozen_string_literal: true

require 'bundler/gem_tasks'
require 'rake/testtask'

Rake::TestTask.new(:test) do |t|
  t.libs << 'test'
  t.libs << 'lib'
  t.test_files = FileList['test/**/*_test.rb']
end

require 'rubocop/rake_task'

RuboCop::RakeTask.new

require 'rake/extensiontask'

Rake::ExtensionTask.new('core') do |ext|
  ext.lib_dir = 'lib/tree_sitter'
  ext.source_pattern = '**/*.{c,cc,cpp}'
end

task :console do
  exec 'irb -I lib -r tree_sitter'
end

task default: %i[test rubocop]

LANGUAGES = %w[javascript python go ruby java
               typescript bash haskell c-sharp cpp agda
               php scala swift verilog c rust css ocaml json
               regex html julia ql tsq jsdoc]


def transform_file(filename, &block)
  content = File.read(filename)
  content = block[content]
  File.write(filename, content)
end

LANGUAGES.each do |language|
  language_underscore = language.gsub('-', '_')
  Rake::ExtensionTask.new(language_underscore) do |ext|
    ext.lib_dir = 'lib/tree_sitter'
  end
end


template_dir = File.join(__dir__, 'ext', 'template')
LANGUAGES.each_with_index do |language, language_index|
  language_underscore = language.gsub('-', '_')
  dest_dir = File.join(__dir__, 'ext', language_underscore)
  task :"download_languages:#{language_underscore}" do
    require 'net/http'
    url = "https://raw.githubusercontent.com/tree-sitter/tree-sitter-#{language}"
    cp_r File.join(__dir__, 'ext', 'template', '.'), dest_dir
    %w[src/parser.c src/parser.cc src/scanner.c src/scanner.cc src/tree_sitter/parser.h src/tag.h].each do |filename|
      file_url = "#{url}/master/#{filename}"
      response = Net::HTTP.get_response(URI.parse(file_url))
      if response.code == "200"
        dest_filename = File.join(dest_dir, filename.sub('src/', ''))
        mkdir_p File.dirname(dest_filename)
        File.write(dest_filename, response.body)
      else
        puts "Failed to fetch #{filename} (#{response.code})"
      end
    end
  end

  task :"create_language_module:#{language_underscore}" do
    %w[extconf.rb template.c].each do |filename|
      path = File.join(dest_dir, filename)
      orig_path = File.join(template_dir, filename)
      cp orig_path, path
      transform_file(path) do |content|
        content.gsub('template', language.gsub('-', '_'))
               .gsub('Template', language.split('-').map(&:capitalize).join(''))
               .gsub('TEMPLATE', language.gsub('-', '_').upcase)
               .gsub('LANGUAGE_ID', language_index.to_s)
      end
    end
    mv File.join(dest_dir, 'template.c'), File.join(dest_dir, "#{language_underscore}.c")
  end
end

task :gen_language_ids_header do
  filename = File.join(__dir__, 'ext', 'core', 'language_ids.h')
  language_ids = <<~CODE
    #pragma once
    typedef enum {
      #{ LANGUAGES.map.with_index { "LANGUAGE_#{_1.gsub('-', '_').upcase} = #{_2}" }.join(",\n  ") }
    } LanguageId;
  CODE
  File.write(filename, language_ids)
end

task :download_languages => LANGUAGES.map { :"download_languages:#{_1.gsub('-', '_')}" }
task :create_language_modules => LANGUAGES.map { :"create_language_module:#{_1.gsub('-', '_')}" }

task :update_tree_sitter do
  require 'tempfile'
  Dir.mktmpdir do |dir|
    sh "git clone --depth 1 https://github.com/tree-sitter/tree-sitter #{dir}"
    core_dir = File.join(__dir__, 'ext', 'core')
    vendor_dir = File.join(core_dir, 'vendor')
    mkdir_p vendor_dir
    %w[src include].each do |sub_dir|
      src_dir = File.join(dir, 'lib', sub_dir)
      dest_dir = File.join(vendor_dir, sub_dir)
      cp_r File.join(src_dir, '.'), dest_dir
    end

    mv File.join(vendor_dir, 'src', 'lib.c'), core_dir
    transform_file(File.join(core_dir, 'lib.c')) do |content|
      content.gsub("./", "vendor/src/")
             .sub("#include", %Q{#include "alloc.h"\n#include})
    end
  end
end
