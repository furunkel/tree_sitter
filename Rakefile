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

Rake::ExtensionTask.new('core')
Rake::ExtensionTask.new('javascript')

task default: %i[test rubocop]

LANGUAGES = %w[javascript]

def transform_file(filename, &block)
  content = File.read(filename)
  content = block[content]
  File.write(filename, content)
end

task :download_languages do
  require 'net/http'
  LANGUAGES.each do |language|
    url = "https://raw.githubusercontent.com/tree-sitter/tree-sitter-#{language}"
    dest_dir = File.join(__dir__, 'ext', language)
    cp_r File.join(__dir__, 'ext', 'template', '.'), dest_dir
    %w[src/parser.c src/parser.cc src/scanner.c src/scanner.cc src/tree_sitter/parser.h].each do |filename|
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

    %w[extconf.rb template.c].each do |filename|
      path = File.join(dest_dir, filename)
      transform_file(path) do |content|
        ontent.gsub('template', language)
              .gsub('Template', language.capitalize)
              .gsub('TEMPLATE', language.upcase)
      end
    end
    mv File.join(dest_dir, 'template.c'), File.join(dest_dir, "#{language}.c")

  end
end

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
