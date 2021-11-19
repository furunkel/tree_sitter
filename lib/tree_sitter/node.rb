require 'tree_sitter/core'

module TreeSitter
  class Node
    def cursor
      Tree::Cursor.new self
    end
  end
end