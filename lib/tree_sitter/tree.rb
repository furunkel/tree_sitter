require 'tree_sitter/core'

module TreeSitter
  class Tree
    EXTENSION_MAP = {
      '.agda' => 'agada',
      '.sh' => 'bash',
      '.bash' => 'bash',
      '.c' => 'c',
      '.cpp' => 'cpp',
      '.cxx' => 'cpp',
      '.cc' => 'cpp',
      '.cs' => 'c_sharp',
      '.css' => 'css',
      '.go' => 'go',
      '.hs' => 'haskell',
      '.html' => 'html',
      '.java' => 'java',
      '.js' => 'javascript',
      '.json' => 'json',
      '.jl' => 'julia',
      '.ml' => 'ocaml',
      '.mli' => 'ocaml',
      '.php' => 'php',
      '.py' => 'python',
      '.ql' => 'ql',
      '.rb' => 'ruby',
      '.rs' => 'rust',
      '.scala' => 'scala',
      '.swift' => 'swift',
      '.ts' => 'typescript',
      '.tsq' => 'tsq'
      #'' => 'jsdoc',
      #'' => 'regex',
      #'' => 'verilog'
    }.freeze

    class << self
      def for_filename(file_or_filename)
        filename =
          case file_or_filename
          when String
            file_or_filename
          when file_or_filename.respond_to?(:to_path)
            file_or_filename.to_path
          else
            raise ArgumentError, 'must pass string or file'
          end

        extension = File.extname(filename)
        language = EXTENSION_MAP[extension]
        raise "unknown file extension #{extension}" if language.nil?

        require "tree_sitter/#{language}"
        class_name = language.split('_').map(&:capitalize).join
        TreeSitter.const_get(class_name)
      end

      def parse_file(file_or_filename, **kw_args)
        case file_or_filename
        when String
          filename = file_or_filename
          content = File.read(file_or_filename)
        when file_or_filename.respond_to?(:to_path)
          filename = file_or_filename.to_path
          content = file_or_filename.read
        else
          raise ArgumentError, 'must pass string or file'
        end
        for_filename(filename).parse content, **kw_args
      end
    end

    def fringe(nodes: true, types: false, comments: true, whitespace: false)
      __fringe__ nodes, types, comments, whitespace
    end

    def to_h
      __to_h__
    end

    def cursor
      root_node.cursor
    end

  end
end