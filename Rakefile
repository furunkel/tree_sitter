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

DIALECTS = {
  'php' => ['php', 'php_only']
}

# maps dialects to ids
# if language has no dialects, its only dialect is the language itself in snake case
LANGUAGE_IDS = LANGUAGES.flat_map { DIALECTS.fetch(_1, [_1]).map {|d| d.gsub('-', '_') } }.each_with_index.to_h 

def transform_file(filename, &block)
  content = File.read(filename)
  content = block[content]
  File.write(filename, content)
end

LANGUAGE_IDS.each_key do |dialect|
  Rake::ExtensionTask.new(dialect) do |ext|
    ext.lib_dir = 'lib/tree_sitter'
  end
end

def fix_includes(file_content)
  # this only appears in multi-dialect languages (currently PHP) that share common code in headers
  file_content.gsub('#include "../../common/scanner.h"', '#include "common/scanner.h"')
end

def download_file(language, dialect, filename, dest_dir)
  require 'net/http'
  url = "https://raw.githubusercontent.com/tree-sitter/tree-sitter-#{language}"
  file_url = "#{url}/master/#{dialect ? "#{dialect}/" : '' }#{filename}"
  response = Net::HTTP.get_response(URI.parse(file_url))
  if response.code == "200"
    dest_filename = File.join(dest_dir, filename.sub('src/', ''))
    mkdir_p File.dirname(dest_filename)
    File.write(dest_filename, fix_includes(response.body))
  else
    puts "Failed to fetch #{filename} (#{response.code})"
  end
end

LANGUAGE_FILES = %w[src/parser.c
                  src/parser.cc
                  src/scanner.c
                  src/scanner.cc
                  src/tree_sitter/parser.h
                  src/tag.h
                ].freeze

template_dir = File.join(__dir__, 'ext', 'template')
LANGUAGES.each_with_index do |language, language_index|
  language_underscore = language.gsub('-', '_')
  task :"download_languages:#{language_underscore}" do

    dialects = DIALECTS.fetch(language, [nil])

    dialects.each do |dialect|
      dest_dir = File.join(__dir__, 'ext', dialect || language_underscore)
      download_file(language, nil, 'common/scanner.h', dest_dir) if dialects.size > 1
      cp_r File.join(__dir__, 'ext', 'template', '.'), dest_dir
      LANGUAGE_FILES.each do |filename|
        download_file(language, dialect, filename, dest_dir)
      end
    end
  end

  task :"create_language_module:#{language_underscore}" do
    dialects = DIALECTS.fetch(language, [language_underscore])
    dialects.each do |dialect|
      dest_dir = File.join(__dir__, 'ext', dialect)
      %w[extconf.rb template.c].each do |filename|
        path = File.join(dest_dir, filename)
        orig_path = File.join(template_dir, filename)
        cp orig_path, path
        transform_file(path) do |content|
          content.gsub('template', dialect)
                .gsub('Template', dialect.split('_').map(&:capitalize).join(''))
                .gsub('TEMPLATE', dialect.upcase)
                .gsub('LANGUAGE_ID', LANGUAGE_IDS.fetch(dialect).to_s)
        end
      end
      mv File.join(dest_dir, 'template.c'), File.join(dest_dir, "#{dialect}.c")
    end
  end
end

task :gen_language_ids_header do
  filename = File.join(__dir__, 'ext', 'core', 'language_ids.h')
  language_ids = <<~CODE
    #pragma once
    typedef enum {
      #{ LANGUAGE_IDS.map { "LANGUAGE_#{_1.gsub('-', '_').upcase} = #{_2}" }.join(",\n  ") }
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
